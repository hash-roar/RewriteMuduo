#pragma once
#include <atomic>

#include "base/Common.h"
#include "network/Callback.h"
#include "unix/Time.h"
namespace rnet::network {
// using namespace rnet::Unix;  //?
// using namespace rnet;
class Timer : rnet::Noncopyable {
 public:
  Timer(network::TimerCallback cb, Unix::Timestamp when, double interval)
      : callback(std::move(cb)),
        expiration_(when),
        interval(interval),
        repeat(interval > 0.0),
        sequence(sNumCreated.fetch_add(1)) {}

  void Run() const { callback(); }

  Unix::Timestamp Expiration() const { return expiration_; }
  bool Repeat() const { return repeat; }
  int64_t Sequence() const { return sequence; }

  void Restart(rnet::Unix::Timestamp now);

  static int64_t NumCreated() { return sNumCreated.load(); }

 private:
  const network::TimerCallback callback;
  Unix::Timestamp expiration_;
  const double interval;
  const bool repeat;
  const int64_t sequence;

  static std::atomic_int64_t sNumCreated;
};

}  // namespace rnet::Network