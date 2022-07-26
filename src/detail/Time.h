#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "Common.h"

namespace rnet::detail {
class Timestamp {
 public:
  ///
  /// Constructs an invalid Timestamp.
  ///
  Timestamp() : microSecondsSinceEpoch_(0) {}

  ///
  /// Constructs a Timestamp at specific time
  ///
  /// @param microSecondsSinceEpoch
  explicit Timestamp(int64_t microSecondsSinceEpochArg)
      : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

  void swap(Timestamp& that) {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  // default copy/assignment/dtor are Okay

  std::string toString() const;
  std::string toFormattedString(bool showMicroseconds = true) const;

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  SysTimep_t toChronoSysTime() const {
    return SysTimep_t{std::chrono::microseconds{microSecondsSinceEpoch_}};
  }
  // for internal usage.
  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(microSecondsSinceEpoch_ /
                               kMicroSecondsPerSecond);
  }

  ///
  /// Get time of now.
  ///
  static Timestamp now();
  static Timestamp invalid() { return Timestamp(); }

  static Timestamp fromUnixTime(time_t t) { return fromUnixTime(t, 0); }

  static Timestamp fromUnixTime(time_t t, int microseconds) {
    return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond +
                     microseconds);
  }

  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
  int64_t microSecondsSinceEpoch_;
};
}  // namespace rnet::detail