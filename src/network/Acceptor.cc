#include "network/EventLoop.h"
#include "network/Acceptor.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

#include "log/Logger.h"
#include "network/NetAddress.h"
#include "network/SocketOps.h"
namespace rnet::network {
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr,
                   bool reuseport)
    : loop_(loop),
      acceptSocket_(sockets::CreateNonblockingOrDie(listenAddr.Family())),
      acceptChannel_(loop, acceptSocket_.Fd()),
      listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idleFd_ >= 0);
  acceptSocket_.SetReuseAddr(true);
  acceptSocket_.SetReusePort(reuseport);
  acceptSocket_.BindAddress(listenAddr);
  acceptChannel_.SetReadCallback(std::bind(&::rnet::network::Acceptor::HandleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.DisableAll();
  acceptChannel_.Remove();
  ::close(idleFd_);
}

void Acceptor::Listen() {
  loop_->AssertInLoopThread();
  listening_ = true;
  acceptSocket_.Listen();
  acceptChannel_.EnableReading();
}

void Acceptor::HandleRead() {
  loop_->AssertInLoopThread();
  InetAddress peerAddr;
  // FIXME loop until no more
  int connfd = acceptSocket_.Accept(&peerAddr);
  if (connfd >= 0) {
    // string hostport = peerAddr.ToIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      sockets::Close(connfd);
    }
  } else {
    LOG_SYSERR << "in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE) {
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.Fd(), nullptr, nullptr);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

}  // namespace rnet::Network