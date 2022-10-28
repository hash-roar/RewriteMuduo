#include "network/EventLoop.h"
#include "network/TimerQueue.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <tuple>
#include "log/Logger.h"
namespace rnet::network {
namespace detail {

int CreateTimerFd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec HowMuchTimeFromNow(Unix::Timestamp when) {
  int64_t microseconds = when.MicroSecondsSinceEpoch() -
                         Unix::Timestamp::Now().MicroSecondsSinceEpoch();
  if (microseconds < 100) {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(microseconds /
                                  Unix::Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Unix::Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void ReadTimerfd(int timerfd, Unix::Timestamp now) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at "
            << now.ToString();
  if (n != sizeof howmany) {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n
              << " bytes instead of 8";
  }
}

void ResetTimerfd(int timerfd, Unix::Timestamp Expiration) {
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  MemZero(&newValue, sizeof newValue);
  MemZero(&oldValue, sizeof oldValue);
  newValue.it_value = HowMuchTimeFromNow(Expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret) {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}  // namespace detail

using namespace detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd(CreateTimerFd()),
      timerfdChannel_(loop, timerfd),
      timers_(),
      callingExpiredTimers_(false) {
  timerfdChannel_.SetReadCallback(std::bind(&::rnet::network::TimerQueue::HandleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfdChannel_.EnableReading();
}

TimerQueue::~TimerQueue() {
  timerfdChannel_.DisableAll();
  timerfdChannel_.Remove();
  ::close(timerfd);
  // do not remove channel, since we're in EventLoop::dtor();
  for (const Entry& timer : timers_) {
    delete timer.second;
  }
}

TimerId TimerQueue::AddTimer(TimerCallback cb, Unix::Timestamp when,
                             double interval) {
  auto timer = new Timer(std::move(cb), when, interval);
  loop_->RunInLoop(std::bind(&::rnet::network::TimerQueue::AddTimerInLoop, this, timer));
  return TimerId(timer, timer->Sequence());
}

void TimerQueue::Cancel(TimerId timerId) {
  loop_->RunInLoop(std::bind(&::rnet::network::TimerQueue::CancelInLoop, this, timerId));
}

void TimerQueue::AddTimerInLoop(Timer* timer) {
  loop_->AssertInLoopThread();
  bool earliestChanged = Insert(timer);

  if (earliestChanged) {
    ResetTimerfd(timerfd, timer->Expiration());
  }
}

void TimerQueue::CancelInLoop(TimerId timerId) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.Sequence_);
  auto itr = activeTimers_.find(timer);
  if (itr != activeTimers_.end()) {
    size_t n = timers_.erase(Entry(itr->first->Expiration(), itr->first));
    assert(n == 1);
    delete itr->first;  // FIXME: no delete please
    activeTimers_.erase(itr);
  } else if (callingExpiredTimers_) {
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::HandleRead() {
  loop_->AssertInLoopThread();
  Unix::Timestamp now(Unix::Timestamp::Now());
  ReadTimerfd(timerfd, now);

  std::vector<Entry> expired = GetExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (const Entry& it : expired) {
    it.second->Run();
  }
  callingExpiredTimers_ = false;

  Reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::GetExpired(Unix::Timestamp now) {
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  auto end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->Sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1);
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}

void TimerQueue::Reset(const std::vector<Entry>& expired, Unix::Timestamp now) {
  Unix::Timestamp nextExpire;

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->Sequence());
    if (it.second->Repeat() &&
        cancelingTimers_.find(timer) == cancelingTimers_.end()) {
      it.second->Restart(now);
      Insert(it.second);
    } else {
      // FIXME move to a free list
      delete it.second;  // FIXME: no delete please
    }
  }

  if (!timers_.empty()) {
    nextExpire = timers_.begin()->second->Expiration();
  }

  if (nextExpire.Valid()) {
    ResetTimerfd(timerfd, nextExpire);
  }
}

bool TimerQueue::Insert(Timer* timer) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  bool earliestChanged = false;
  Unix::Timestamp when = timer->Expiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliestChanged = true;
  }
  {
    // auto result = timers_.insert(Entry(when, timer));
    auto [_ignore, success] = timers_.insert(Entry(when, timer));
    assert(success);
  }
  {
    auto [_ignore, success] =
        activeTimers_.insert(ActiveTimer(timer, timer->Sequence()));
    assert(success);
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}

}  // namespace rnet::Network