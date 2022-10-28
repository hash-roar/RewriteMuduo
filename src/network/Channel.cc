#include "network/Channel.h"

#include <sys/epoll.h>

#include <cassert>
#include <sstream>

#include "log/Logger.h"
#include "network/EventLoop.h"

namespace rnet::network {
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;
Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd(fd) {}

Channel::~Channel() {
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->IsInLoopThread()) {
    assert(!loop_->HasChannel(this));
  }
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::Update() {
  addedToLoop_ = true;
  loop_->UpdateChannel(this);
}

void Channel::remove() {
  assert(IsNoneEvent());
  addedToLoop_ = false;
  loop_->RemoveChannel(this);
}

void Channel::handleEvent(Unix::Timestamp receiveTime) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(receiveTime);
    }
  } else {  //?
    HandleEventWithGuard(receiveTime);
  }
}

void Channel::HandleEventWithGuard(Unix::Timestamp receiveTime) {
  eventHandling_ = true;
  LOG_TRACE << ReventsToString();
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
    if (logHup_) {
      LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if (closeCallback_) { closeCallback_();
}
  }

  //   if (revents_ & POLLNVAL) {
  //     LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
  //   }

  if (revents_ & (EPOLLERR)) {
    if (errorCallback_) { errorCallback_();
}
  }
  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    if (readCallback_) { readCallback_(receiveTime);
}
  }
  if (revents_ & EPOLLOUT) {
    if (writeCallback_) { writeCallback_();
}
  }
  eventHandling_ = false;
}

std::string Channel::reventsToString() const {
  return eventsToString(fd_, revents_);
}

std::string Channel::eventsToString() const {
  return eventsToString(fd_, events_);
}

std::string Channel::eventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & EPOLLIN) { oss << "IN ";
}
  if (ev & EPOLLPRI) { oss << "PRI ";
}
  if (ev & EPOLLOUT) { oss << "OUT ";
}
  if (ev & EPOLLHUP) { oss << "HUP ";
}
  if (ev & EPOLLRDHUP) { oss << "RDHUP ";
}
  if (ev & EPOLLERR) { oss << "ERR ";
}
  //   if (ev & POLLNVAL)
  //     oss << "NVAL ";

  return oss.str();
}

}  // namespace rnet::Network
