#pragma once
#include <functional>
#include <memory>

#include "base/Common.h"
#include "network/NetAddress.h"
namespace rnet::network {
class Channel;
class EventLoop;

class Connector : Noncopyable, public std::enable_shared_from_this<Connector> {
 public:
  using NewConnectionCallback = std::function<void(int)>;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
  }

  void Start();    // can be called in any thread
  void Restart();  // must be called in loop thread
  void Stop();     // can be called in any thread

  const InetAddress& ServerAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30 * 1000;
  static const int kInitRetryDelayMs = 500;

  void SetState(States s) { state_ = s; }
  void StartInLoop();
  void StopInLoop();
  void Connect();
  void Connecting(int sockfd);
  void HandleWrite();
  void HandleError();
  void Retry(int sockfd);
  int RemoveAndResetChannel();
  void ResetChannel();

  EventLoop* loop_;
  InetAddress serverAddr_;
  bool connect_;  // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
};

}  // namespace rnet::Network