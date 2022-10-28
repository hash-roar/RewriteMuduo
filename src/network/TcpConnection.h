#pragma once
#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <netinet/tcp.h>

#include "base/Common.h"
#include "file/ConnBuffer.h"
#include "network/Callback.h"
#include "network/NetAddress.h"
namespace rnet::network {

class Channel;
class EventLoop;
class Socket;

// TcpConnection
// 是一个tcp链接的封装,由智能指针管理.类的生命周期与tcp链接的生命周期相同.
// TcpConnection
// 拥有输入输出缓冲区,socket,channel的值语义.在链接析构时指针指针会调用
// socket的析构函数释放文件描述符.只要在链接的生命周期类对buffer进行的读写都是有效的.
class TcpConnection : Noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  // (从服务端来看)构造函数在acceptor调用callback后在tcp server 的new
  // connection函数中调用,并
  //在tcp server 中存储tcp connection的智能指针
  TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                const InetAddress& localAddr, const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* GetLoop() const { return loop_; }
  const std::string& Name() const { return name; }
  const InetAddress& LocalAddress() const { return localAddr; }
  const InetAddress& PeerAddress() const { return peerAddr; }
  // 获取状态机状态
  bool Connected() const { return state_ == kConnected; }
  bool Disconnected() const { return state_ == kDisconnected; }
  // return true if success.
  bool GetTcpInfo(struct tcp_info*) const;
  std::string GetTcpInfoString() const;

  // void send(string&& message); // C++11
  void Send(const void* message, int len);
  void Send(std::string_view message);
  // void send(Buffer&& message); // C++11
  void Send(file::Buffer* message);  // this one will swap data
  void Shutdown();                   // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no
  // simultaneous calling
  void ForceClose();
  void ForceCloseWithDelay(double seconds);
  void SetTcpNoDelay(bool on);
  // reading or not
  void StartRead();
  void StopRead();
  bool IsReading() const {
    return reading_;
  };  // NOT thread safe, may race with start/stopReadInLoop

  // 出现了复制构造,考虑移动语义?
  void SetContext(const std::any& context) { context_ = context; }

  const std::any& GetContext() const { return context_; }

  std::any* GetMutableContext() { return &context_; }

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
  }

  void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                size_t highWaterMark) {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
  }

  /// Advanced interface
  file::Buffer* InputBuffer() { return &inputBuffer_; }

  file::Buffer* OutputBuffer() { return &outputBuffer_; }

  /// Internal use only.
  void SetCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void ConnectEstablished();  // should be called only once
  // called when TcpServer has removed me from its map
  void ConnectDestroyed();  // should be called only once

 private:
  // 内部状态机:
  // kConnecting: 在tcp connection构造时为此状态,此时tcp connection
  // 的初始化还没有完全完成, 即在构造函数和connectEstablished函数调用之间.
  // kConnected: tcp server调用new connection
  // 中注册connectEstablished然后调用,状态机状态改变
  // kDisconnecting: 调用shutdown 或者 forceClose
  // 但真正的shutdown,close还没有调用时(因为只是将相应操作注册到eventloop中)
  // kDisconnected: 在主动关闭的handleclose
  // 或者被动调用的connectionDestroyed到连接真正析构的一段时间内
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

  void HandleRead(Unix::Timestamp receiveTime);
  void HandleWrite();
  void HandleClose();
  void HandleError();
  // void sendInLoop(string&& message);
  void SendInLoop(std::string_view message);
  void DoSendInLoop(const void* message, size_t len);
  void ShutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void ForceCloseInLoop();
  void SetState(StateE s) { state_ = s; }
  const char* StateToString() const;
  void StartReadInLoop();
  void StopReadInLoop();

  EventLoop* loop_;
  const std::string name;
  StateE state_;  // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr;
  const InetAddress peerAddr;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;
  file::Buffer inputBuffer_;
  file::Buffer outputBuffer_;  // FIXME: use list<Buffer> as output buffer.
  std::any context_;
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}  // namespace rnet::Network
