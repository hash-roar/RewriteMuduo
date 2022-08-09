#include "network/Timer.h"

#include <atomic>

namespace rnet::Network {
std::atomic_int64_t Timer::sNumCreated{0};

void Timer::restart(Unix::Timestamp now) {
  if (repeat_) {
    expiration_ = Unix::addTime(now, interval_);
  } else {
    expiration_ = Unix::Timestamp::invalid();
  }
}
}  // namespace rnet::Network