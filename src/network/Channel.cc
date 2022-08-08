#include "network/Channel.h"

#include "network/EventLoop.h"

namespace rnet::Network {
Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}


}  // namespace rnet::Network
