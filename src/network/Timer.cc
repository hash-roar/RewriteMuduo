#include "network/Timer.h"

#include <atomic>

namespace rnet::network {
std::atomic_int64_t Timer::sNumCreated{0};

void Timer::Restart(Unix::Timestamp now) {
  if (repeat) {
    expiration_ = Unix::AddTime(now, interval);
  } else {
    expiration_ = Unix::Timestamp::Invalid();
  }
}
}  // namespace rnet::Network