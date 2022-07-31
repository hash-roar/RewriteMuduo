#pragma once
#include <map>
#include <vector>

#include "base/Common.h"
#include "network/EventLoop.h"
#include "unix/Time.h"
namespace rnet::Network {
class Channel;

class Epoll : noncopyable {
  using ChannelList = std::vector<Channel*>;

 public:
  Epoll(EventLoop* loop);
  ~Epoll();

  Unix::Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);
  bool hasChannel(Channel* channel);
  static Epoll newPoller(EventLoop* loop);

  void assertInLoopThread() const { ownerLoop_->assertInLoopThread(); }

 private:
  using ChannelMap = std::map<int, Channel*>;
  using EventList = std::vector<struct epoll_event>;
  
  static const int kInitEventListSize = 16;
  static const char* operationToString(int op);

  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  int epollfd_;

  ChannelMap channels_;
  EventList events_;
  EventLoop* ownerLoop_;
};

}  // namespace rnet::Network