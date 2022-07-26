#pragma once
#include <memory>
#include <set>

#include "base/Common.h"
#include "network/Callback.h"
#include "network/Channel.h"
namespace rnet::network {
class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : Noncopyable {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId AddTimer(TimerCallback cb, Unix::Timestamp when, double interval);

  void Cancel(TimerId timerId);

 private:
  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  using Entry = std::pair<Unix::Timestamp, Timer*>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer*, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

  void AddTimerInLoop(Timer* timer);
  void CancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void HandleRead();
  // move out all expired timers
  std::vector<Entry> GetExpired(Unix::Timestamp now);
  void Reset(const std::vector<Entry>& expired, Unix::Timestamp now);

  bool Insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd;
  network::Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */
  ActiveTimerSet cancelingTimers_;
};
}  // namespace rnet::Network