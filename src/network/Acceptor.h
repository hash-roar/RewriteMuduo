#pragma once
#include <functional>

#include "base/Common.h"
#include "network/Channel.h"
#include "network/Socket.h"
namespace rnet::Network {
class EventLoop;
class InetAddress;
class Acceptor : noncopyable {
 public:
  using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();
  void setNewConnectionCallback(const NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
  }
  void listen();

  bool listening() const { return listening_; }

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
  int idleFd_;
};
}  // namespace rnet::Network