#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <memory>

#include "base/Common.h"
#include "network/Callback.h"
#include "network/NetAddress.h"
namespace rnet::network {
class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : Noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;
  enum Option {
    kNoReusePort,
    kReusePort,
  };

  // TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop, const InetAddress& listenAddr,
            const std::string& nameArg, Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

  const std::string& IpPort() const { return ipPort; }
  const std::string& Name() const { return name; }
  EventLoop* GetLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void SetThreadNum(int numThreads);
  void SetThreadInitCallback(const ThreadInitCallback& cb) {
    threadInitCallback_ = cb;
  }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> ThreadPool() { return threadPool_; }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void Start();

  /// Set connection callback.
  /// Not thread safe.
  //
  void SetConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
  }

  /// Set message callback.
  /// Not thread safe.
  void SetMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
  }

 private:
  /// Not thread safe, but in loop
  void NewConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  void RemoveConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  EventLoop* loop_;  // the acceptor loop
  const std::string ipPort;
  const std::string name;
  std::unique_ptr<Acceptor> acceptor_;  // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  // 连接发生变化时回调函数
  ConnectionCallback connectionCallback_;
  //消息到来时(消息读到buffer中后)回调
  MessageCallback messageCallback_;
  // 写完成,即output缓冲区中数据发送完毕
  WriteCompleteCallback writeCompleteCallback_;
  // io线程初始化完成后的回调
  ThreadInitCallback threadInitCallback_;
  std::atomic_int32_t started_{0};
  // always in loop thread
  int nextConnId_;
  ConnectionMap connections_;
};
}  // namespace rnet::Network