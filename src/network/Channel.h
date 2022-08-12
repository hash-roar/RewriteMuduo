#pragma once
#include <functional>

#include "base/Common.h"
#include "unix/Time.h"
namespace rnet::Network {

class EventLoop;

// channel 没有套接字的值语义,epoll与tcp connection之间的交换
// 交互过程为 tcp connection通过eventloop将事件注册到epoll中,然后epoll事件来临时
// epoll 将对应事件添加到activeChannels集合中.然后event
// loop负责调用每个channel的handleEvent函数 在这个函数中,tcp
// connection注册的回调被调用,从而用户注册的回调也被调用
class Channel : noncopyable {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Unix::Timestamp)>;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void handleEvent(Unix::Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }  // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() {
    events_ |= kReadEvent;
    update();
  }
  void disableReading() {
    events_ &= ~kReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  std::string reventsToString() const;
  std::string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  static std::string eventsToString(int fd, int ev);

  void update();
  void handleEventWithGuard(Unix::Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  const int fd_;
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