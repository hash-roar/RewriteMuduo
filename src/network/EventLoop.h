#pragma once
#include <any>
#include <functional>
#include <mutex>

#include "base/Common.h"
#include "network/Channel.h"
#include "network/Timer.h"
#include "network/TimerId.h"
#include "unix/Thread.h"
#include "unix/Time.h"
namespace rnet::Network {

class Epoll;
class Channel;
class TimerQueue;
class EventLoop : Noncopyable {
 public:
  // typedef std::function<void()> Functor;
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for std::unique_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void Loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void Quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Unix::Timestamp PollReturnTime() const { return pollReturnTime_; }

  int64_t Iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void RunInLoop(Functor cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void QueueInLoop(Functor cb);

  size_t QueueSize() const;

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  network::TimerId RunAt(Unix::Timestamp time, network::TimerCallback cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  network::TimerId RunAfter(double delay, network::TimerCallback cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  network::TimerId RunEvery(double interval, network::TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void Cancel(network::TimerId timerId);

  // internal usage
  void Wakeup();
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }
  bool IsInLoopThread() const { return threadId == thread::Tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool EventHandling() const { return eventHandling_; }

  void SetContext(const std::any& context) { context_ = context; }

  const std::any& GetContext() const { return context_; }

  std::any* GetMutableContext() { return &context_; }

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  void HandleWakeUp();  // waked up
  void DoPendingFunctors();

  void PrintActiveChannels() const;  // DEBUG

  using ChannelList = std::vector<Channel*>;

  bool looping_; /* atomic */
  std::atomic<bool> quit_;
  bool eventHandling_;          /* atomic */
  bool callingPendingFunctors_; /* atomic */
  int64_t iteration_;
  const pid_t threadId;
  Unix::Timestamp pollReturnTime_;
  std::unique_ptr<Epoll> poller_;
  std::unique_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeupChannel_;
  std::any context_;

  // scratch variables
  ChannelList activeChannels_;
  Channel* currentActiveChannel_;

  mutable std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;
};
}  // namespace rnet::Network