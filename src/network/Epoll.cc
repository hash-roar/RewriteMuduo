#include "network/Epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "log/Logger.h"

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

using namespace rnet::Network;

Epoll::Epoll(EventLoop* loop)
    : ownerLoop_(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG_SYSFATAL << "Epoll::Epoll";
  }
}

Epoll::~Epoll() {
  ::close(epollfd_);
  LOG_TRACE << "close epoll:" << epollfd_;
}

