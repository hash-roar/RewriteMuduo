#pragma once

#include <cstdint>
namespace rnet::network {

class Timer;

class TimerId {
 public:
  TimerId() : timer_(nullptr), sequence_(0) {}

  TimerId(Timer* timer, int64_t seq) : timer_(timer), sequence_(seq) {}

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;

 private:
  Timer* timer_;
  int64_t sequence_;
};
}  // namespace rnet::Network