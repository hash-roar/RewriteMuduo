#include "network/TcpConnection.h"

#include <string>
#include <string_view>

#include "file/ConnBuffer.h"
#include "log/Logger.h"
#include "network/Callback.h"
#include "network/Channel.h"
#include "network/EventLoop.h"
#include "network/Socket.h"
#include "unix/Thread.h"

using namespace rnet;
using namespace rnet::Unix;
using namespace rnet::File;

namespace rnet::Network {

// 参数中的loop为io线程中的eventloop ,不一定是main loop(通过round robin
// 算法确定) .在构造函数中设置事件的回调,在回调过程中tcp
// connection一直存在只用绑定普通指针即可
// tcp connection在这些回调里真正调用用户注册的回调
TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg,
                             int sockfd, const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) {
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
            << " fd=" << sockfd;

  // 可以加一个config 而不是默认开启
  // keep alive 是tcp 发送一个没有数据的包等待对方返回一个ack来查看对端是否活跃
  // 能防止防火墙关闭长时间不活跃的连接
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
            << " fd=" << channel_->fd() << " state=" << stateToString();
  assert(state_ == kDisconnected);
  // 确保connection是经过connectionDestroyed函数关闭的,否则会存在错误的智能指针
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const {
  return socket_->getTcpInfo(tcpi);
}

std::string TcpConnection::getTcpInfoString() const {
  char buf[1024];
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof buf);
  return buf;
}

//发送数据,数据的生存周期由自己保证.
//一般是栈数据,但是send时会将数据拷贝到io loop
void TcpConnection::send(const void* data, int len) {
  send(std::string_view(static_cast<const char*>(data), len));
}

void TcpConnection::send(std::string_view message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      auto function_pointer = &TcpConnection::sendInLoop;
      //产生复制
      loop_->runInLoop(std::bind(function_pointer,
                                 this,  // FIXME
                                 std::string{message}));
      // 这里发生了数据复制
      // std::forward<string>(message)));
    }
  }
}

void TcpConnection::send(Buffer* buf) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      doSendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    } else {
      auto function_pointer = &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(function_pointer,
                                 this,  // FIXME
                                 buf->retrieveAllAsString()));
      // 生成的临时变量经过移动构造成为生成的仿函数的成员变量
      // 底层数据复制一次
      // std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(std::string_view message) {
  doSendInLoop(message.data(), message.size());
}

// 发送数据的核心逻辑
void TcpConnection::doSendInLoop(const void* data, size_t len) {
  loop_->assertInLoopThread();
  ssize_t write_bytes = 0;
  size_t remaining = len;
  bool faultError = false;
  // 在已经调用了handleClose 或者connectionDestroy
  // 后再发送的行为将被忽略并打印警告
  if (state_ == kDisconnected) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  // 普通发送: 直接调用write写入内核缓冲区
  // 超量发送:
  // 一次性将本套接字的内核缓冲区写满并还有剩余数据未发送,此时将数据暂存至output
  // buffer
  // 上次已经超量发送:
  // 上次超量发送,此时直接跳过直接发送将数据存到发送缓冲区,否则数据可能会乱序
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    write_bytes = Sockets::write(channel_->fd(), data, len);
    if (write_bytes >= 0) {
      remaining = len - write_bytes;
      //如果发送完了
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else  // write_bytes < 0
    {
      write_bytes = 0;

      // 非阻塞套接字写满了返回EWOULDBLOCK,此情况忽略,其余情况为需要log的错误
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET)  // FIXME: any others?
        {
          faultError = true;
        }
      }
    }
  }
  assert(remaining <= len);
  // 发生了错误例如对方已经关闭了连接等情况,跳过发送,下一次轮询发生read
  // bytes为0的情况,连接被关闭
  if (!faultError && remaining > 0) {
    size_t oldLen = outputBuffer_.readableBytes();

    //发送缓冲区中堆积了太多的数据,调用高水位回调函数
    if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ &&
        highWaterMarkCallback_) {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(),
                                   oldLen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(data) + write_bytes,
                         remaining);
    // 由于epoll 是水平触发模式,一直关注写事件会busy loop
    // 因此在发送缓冲区中有数据时才关注写事件,一旦写完立即取消关注
    // 数据会在下一次的轮询中的handleWrite中发送
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

//关闭写端,此时状态为kDisconnecting
void TcpConnection::shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::forceClose() {
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnecting);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    handleClose();
  }
}

// void TcpConnection::forceCloseWithDelay(double seconds) {
//   if (state_ == kConnected || state_ == kDisconnecting) {
//     setState(kDisconnecting);
//     loop_->runAfter(
//         seconds, makeWeakCallback(
//                      shared_from_this(),
//                      &TcpConnection::forceClose));  // not forceCloseInLoop
//                      to
//                                                     // avoid race condition
//   }
// }

const char* TcpConnection::stateToString() const {
  switch (state_) {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

// 关闭tcp delay算法,指将几个小包合成一个大包发送.但是会增加延迟
void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

//关注read事件,下一次轮询即可读到数据
void TcpConnection::startRead() {
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop() {
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading()) {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead() {
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop() {
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading()) {
    channel_->disableReading();
    reading_ = false;
  }
}

// 在连接建立的回调中调用,绑定智能指针用于保活,更新状态机状态
void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

//连接关闭的最后一步,此时只有最后一个智能指针隐式绑定在函数对象内,
//本函数执行完堆内存析构,套接字关闭,文件描述符释放
void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == kConnected) {
    setState(kDisconnected);
    //告诉内核不再关注此描述符事件
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }

  // epoll 里保存了已经注册的channel,因此此时需要删除不用的channel
  channel_->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
  loop_->assertInLoopThread();
  int savedErrno = 0;
  // 读数据会尽量读取数据,最多可读到65536+buffer.size()长度数据
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    // 当对方请求关闭连接时,epoll返回epollin | EPOLLRDHUP
    // 此时服务端直接关闭连接即可
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = Sockets::write(channel_->fd(), outputBuffer_.peek(),
                               outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      //如果发送缓存已经为空,停止关注读事件,否则会busy loop
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        // 直接将回调入栈,而不是直接调用
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        // 在关注写事件时shutdown会被忽略,因此此时要检测是否已经调用过shutdown了,
        // 将没有成功调用的shutdown补回来
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite";
      // if (state_ == kDisconnecting)
      // {
      //   shutdownInLoop();
      // }
    }
  } else {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  // must be the last line
  // 在main loop 中调用callback进行指针擦除等操作
  closeCallback_(guardThis);
}

void TcpConnection::handleError() {
  int err = Sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << Thread::getErrnoMessage(err);
}

}  // namespace rnet::Network

using namespace rnet::Network;

void rnet::Network::defaultConnectionCallback(const TcpConnectionPtr& conn) {
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message
  // callback only.
}

void rnet::Network::defaultMessageCallback(const TcpConnectionPtr&,
                                           File::Buffer* buf, Unix::Timestamp) {
  buf->retrieveAll();
}
