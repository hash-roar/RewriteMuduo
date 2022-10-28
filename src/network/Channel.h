#pragma once
#include <functional>

#include "base/Common.h"
#include "unix/Time.h"
namespace rnet::network {

class EventLoop;

// channel 没有套接字的值语义,epoll与tcp connection之间的交换
// 交互过程为 tcp connection通过eventloop将事件注册到epoll中,然后epoll事件来临时
// epoll 将对应事件添加到activeChannels集合中.然后event
// loop负责调用每个channel的handleEvent函数 在这个函数中,tcp
// connection注册的回调被调用,从而用户注册的回调也被调用
class Channel : Noncopyable {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Unix::Timestamp)>;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void HandleEvent(Unix::Timestamp receiveTime);
  void SetReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void SetWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void SetCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void SetErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void Tie(const std::shared_ptr<void>&);

  int Fd() const { return fd; }
  int Events() const { return events_; }
  void SetRevents(int revt) { revents_ = revt; }  // used by pollers
  // int rEvents() const { return revents_; }
  bool IsNoneEvent() const { return events_ == kNoneEvent; }

  void EnableReading() {
    events_ |= kReadEvent;
    Update();
  }
  void DisableReading() {
    events_ &= ~kReadEvent;
    Update();
  }
  void EnableWriting() {
    events_ |= kWriteEvent;
    Update();
  }
  void DisableWriting() {
    events_ &= ~kWriteEvent;
    Update();
  }
  void DisableAll() {
    events_ = kNoneEvent;
    Update();
  }
  bool IsWriting() const { return events_ & kWriteEvent; }
  bool IsReading() const { return events_ & kReadEvent; }

  // for Poller
  int Index() { return index_; }
  void SetIndex(int idx) { index_ = idx; }

  // for debug
  std::string ReventsToString() const;
  std::string EventsToString() const;

  void DoNotLogHup() { logHup_ = false; }

  EventLoop* OwnerLoop() { return loop_; }
  void Remove();

 private:
  static std::string EventsToString(int fd, int ev);

  void Update();
  void HandleEventWithGuard(Unix::Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  const int fd;
  int events_{0};
  int revents_{0};  // it's the received event types of epoll or poll
  int index_{-1};   // used by Poller.
  bool logHup_{true};

  std::weak_ptr<void> tie_;
  bool tied_{false};
  bool eventHandling_{false};
  bool addedToLoop_{false};
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace rnet::Network