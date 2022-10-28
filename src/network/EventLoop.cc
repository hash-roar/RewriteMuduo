#include "network/EventLoop.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <csignal>
#include <mutex>

#include "log/Logger.h"
#include "network/Epoll.h"
#include "network/SocketOps.h"
#include "network/TimerQueue.h"
#include "unix/Thread.h"

using namespace rnet;
using namespace rnet::log;
namespace {
thread_local Network::EventLoop* tLoopInThisThread = nullptr;
constexpr int kPollTimeMs = 10000;

int CreateEventFd() {
  int eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (eventFd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return eventFd;
}

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};

IgnoreSigPipe initSignal;

}  // namespace

namespace rnet::Network {
EventLoop* EventLoop::GetEventLoopOfCurrentThread() {
  return tLoopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId(thread::Tid()),
      poller_(new Epoll(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(CreateEventFd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
  if (tLoopInThisThread) {
    LOG_FATAL << "Another EventLoop " << tLoopInThisThread
              << " exists in this thread " << threadId_;
  } else {
    tLoopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeUp, this));
  // we are always reading the wakeup fd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << thread::Tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  tLoopInThisThread = nullptr;
}

void EventLoop::Loop() {
  assert(!looping_);
  AssertInLoopThread();
  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;
    if (Logger::logLevel() <= Logger::TRACE) {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true;
    for (Channel* channel : activeChannels_) {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = nullptr;
    eventHandling_ = false;
    doPendingFunctors();
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::Quit() {
  quit_ = true;
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::RunInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::QueueInLoop(Functor cb) {
  {
    std::lock_guard lock{mutex_};
    pendingFunctors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_) {
    wakeup();
  }
}

size_t EventLoop::QueueSize() const {
  std::lock_guard lock{mutex_};
  return pendingFunctors_.size();
}

TimerId EventLoop::RunAt(Unix::Timestamp time, TimerCallback cb) {
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::RunAfter(double delay, TimerCallback cb) {
  Unix::Timestamp time(addTime(Unix::Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::RunEvery(double interval, TimerCallback cb) {
  Unix::Timestamp time(addTime(Unix::Timestamp::now(), interval));
  return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::Cancel(TimerId timerId) { return timerQueue_->cancel(timerId); }

void EventLoop::UpdateChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  AssertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  AssertInLoopThread();
  if (eventHandling_) {
    assert(currentActiveChannel_ == channel ||
           std::find(activeChannels_.begin(), activeChannels_.end(), channel) ==
               activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  AssertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::AbortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << Thread::tid();
}

void EventLoop::Wakeup() {
  uint64_t one = 1;
  ssize_t n = Sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::HandleWakeUp() {
  uint64_t one = 1;
  ssize_t n = Sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one) {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::lock_guard lock{mutex_};
    functors.swap(pendingFunctors_);
  }

  for (const Functor& functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::PrintActiveChannels() const {
  for (const Channel* channel : activeChannels_) {
    LOG_TRACE << "{" << channel->reventsToString() << "} ";
  }
}

}  // namespace rnet::Network