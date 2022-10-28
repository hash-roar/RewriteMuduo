#pragma once
#include <sys/epoll.h>

#include <map>
#include <vector>

#include "base/Common.h"
#include "network/EventLoop.h"
#include "unix/Time.h"
namespace rnet::network {
class Channel;

class Epoll : Noncopyable {
  using ChannelList = std::vector<Channel*>;

 public:
  Epoll(EventLoop* loop);
  ~Epoll();

  Unix::Timestamp Poll(int timeoutMs, ChannelList* activeChannels);
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel) const;
  static Epoll NewPoller(EventLoop* loop);

  void AssertInLoopThread() const { ownerLoop_->AssertInLoopThread(); }

 private:
  using ChannelMap = std::map<int, Channel*>;
  using EventList = std::vector<struct epoll_event>;

  static const int kInitEventListSize = 16;
  static const char* OperationToString(int op);

  void FillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  void Update(int operation, Channel* channel);

  int epollfd_;

  ChannelMap channels_;
  EventList events_;
  EventLoop* ownerLoop_;
};

}  // namespace rnet::Network