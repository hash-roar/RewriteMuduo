#include "network/Epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "log/Logger.h"

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

using namespace rnet::Unix;

namespace rnet::network {

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

bool Epoll::HasChannel(Channel* channel) const {
  AssertInLoopThread();
  auto it = channels_.find(channel->Fd());
  return it != channels_.end() && it->second == channel;
}

Timestamp Epoll::Poll(int timeoutMs, ChannelList* activeChannels) {
  LOG_TRACE << "fd total count " << channels_.size();
  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
                               static_cast<int>(events_.size()), timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::Now());
  if (numEvents > 0) {
    LOG_TRACE << numEvents << " events happened";
    FillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    LOG_TRACE << "nothing happened";
  } else {
    // error happens, log uncommon ones
    if (savedErrno != EINTR) {
      errno = savedErrno;
      LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
}

void Epoll::FillActiveChannels(int numEvents,
                               ChannelList* activeChannels) const {
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i) {
    auto channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->Fd();
    auto it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->SetRevents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void Epoll::Update(int operation, Channel* channel) {
  struct epoll_event event;
  MemZero(&event, sizeof event);
  event.events = channel->Events();
  event.data.ptr = channel;
  int fd = channel->Fd();
  LOG_TRACE << "epoll_ctl op = " << OperationToString(operation)
            << " fd = " << fd << " event = { " << channel->eventsToString()
            << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_SYSERR << "epoll_ctl op =" << OperationToString(operation)
                 << " fd =" << fd;
    } else {
      LOG_SYSFATAL << "epoll_ctl op =" << OperationToString(operation)
                   << " fd =" << fd;
    }
  }
}

void Epoll::UpdateChannel(Channel* channel) {
  AssertInLoopThread();
  const int index = channel->Index();
  LOG_TRACE << "fd = " << channel->Fd() << " events = " << channel->Events()
            << " index = " << index;
  if (index == kNew || index == kDeleted) {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->Fd();
    if (index == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else  // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->SetIndex(kAdded);

    // segment fault here
    // this->update(EPOLL_CTL_ADD, channel);
    Update(EPOLL_CTL_ADD, channel);

  } else {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->Fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->SetIndex(kDeleted);
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void Epoll::RemoveChannel(Channel* channel) {
  AssertInLoopThread();
  int fd = channel->Fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->IsNoneEvent());
  int index = channel->Index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  assert(n == 1);

  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->SetIndex(kNew);
}

const char* Epoll::OperationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}

}  // namespace rnet::Network
