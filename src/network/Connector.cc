#include "network/Connector.h"

#include "log/Logger.h"
#include "network/EventLoop.h"
#include "unix/Thread.h"

namespace rnet::network {
const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs) {
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::Start() {
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));  // FIXME: unsafe
}

void Connector::StartInLoop() {
  loop_->AssertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_) {
    connect();
  } else {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::Stop() {
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));  // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::StopInLoop() {
  loop_->AssertInLoopThread();
  if (state_ == kConnecting) {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

void Connector::Connect() {
  int sockfd = Sockets::createNonblockingOrDie(serverAddr_.family());
  int ret = Sockets::connect(sockfd, serverAddr_.getSockAddr());
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      Sockets::close(sockfd);
      break;

    default:
      LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
      Sockets::close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::Restart() {
  loop_->AssertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void Connector::Connecting(int sockfd) {
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this));  // FIXME: unsafe
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this));  // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();
}

int Connector::RemoveAndResetChannel() {
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->Fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(
      std::bind(&Connector::resetChannel, this));  // FIXME: unsafe
  return sockfd;
}

void Connector::ResetChannel() { channel_.reset(); }

void Connector::HandleWrite() {
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Sockets::getSocketError(sockfd);
    if (err) {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " "
               << Thread::getErrnoMessage(err);
      retry(sockfd);
    } else if (Sockets::isSelfConnect(sockfd)) {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    } else {
      setState(kConnected);
      if (connect_) {
        newConnectionCallback_(sockfd);
      } else {
        Sockets::close(sockfd);
      }
    }
  } else {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::HandleError() {
  LOG_ERROR << "Connector::handleError state=" << state_;
  if (state_ == kConnecting) {
    int sockfd = removeAndResetChannel();
    int err = Sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << Thread::getErrnoMessage(err);
    retry(sockfd);
  }
}

void Connector::Retry(int  /*sockfd*/) {
  Sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_) {
    LOG_INFO << "Connector::retry - Retry connecting to "
             << serverAddr_.ToIpPort() << " in " << retryDelayMs_
             << " milliseconds. ";
    loop_->runAfter(retryDelayMs_ / 1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  } else {
    LOG_DEBUG << "do not connect";
  }
}
}  // namespace rnet::Network