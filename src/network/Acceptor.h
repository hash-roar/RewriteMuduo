#pragma once
#include <functional>

#include "base/Common.h"
#include "network/Channel.h"
#include "network/Socket.h"
namespace rnet::network {
class EventLoop;
class InetAddress;
class Acceptor : Noncopyable {
 public:
  using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();
  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
  }
  void Listen();

  bool Listening() const { return listening_; }

 private:
  void HandleRead();

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
  int idleFd_;
};
}  // namespace rnet::Network