#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "base/Common.h"

namespace rnet::Unix {
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

  detail::SysTimep_t toChronoSysTime() const {
    return detail::SysTimep_t{std::chrono::microseconds{microSecondsSinceEpoch_}};
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

// class TimeZone {
//  public:
//   explicit TimeZone(const char* zonefile);
//   TimeZone(int eastOfUtc, const char* tzname);  // a fixed timezone
//   TimeZone() = default;                         // an invalid timezone

//   // default copy ctor/assignment/dtor are Okay.

//   bool valid() const {
//     // 'explicit operator bool() const' in C++11
//     return static_cast<bool>(data_);
//   }

//   struct tm toLocalTime(time_t secondsSinceEpoch) const;
//   time_t fromLocalTime(const struct tm&) const;

//   // gmtime(3)
//   static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);
//   // timegm(3)
//   static time_t fromUtcTime(const struct tm&);
//   // year in [1900..2500], month in [1..12], day in [1..31]
//   static time_t fromUtcTime(int year, int month, int day, int hour, int
//   minute,
//                             int seconds);

//   struct Data;

//  private:
//   std::shared_ptr<Data> data_;
// };
}  // namespace rnet::unix