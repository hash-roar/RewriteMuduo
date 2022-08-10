#pragma once
#include <atomic>

#include "base/Common.h"
#include "network/Callback.h"
#include "unix/Time.h"
namespace rnet::Network {
// using namespace rnet::Unix;  //?
// using namespace rnet;
class Timer : rnet::noncopyable {
 public:
  Timer(TimerCallback cb, Unix::Timestamp when, double interval)
      : callback_(std::move(cb)),
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        sequence_(sNumCreated.fetch_add(1)) {}

  void run() const { callback_(); }

  Unix::Timestamp expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  void restart(rnet::Unix::Timestamp now);

  static int64_t numCreated() { return sNumCreated.load(); }

 private:
  const TimerCallback callback_;
  Unix::Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static std::atomic_int64_t sNumCreated;
};

}  // namespace rnet::Network