#pragma once

#include <sys/time.h>

#include <cstdint>
namespace rnet::Unix {
class Duration {
 public:
  static const int64_t kNanosecond;   // = 1LL
  static const int64_t kMicrosecond;  // = 1000
  static const int64_t kMillisecond;  // = 1000 * kMicrosecond
  static const int64_t kSecond;       // = 1000 * kMillisecond
  static const int64_t kMinute;       // = 60 * kSecond
  static const int64_t kHour;         // = 60 * kMinute
 public:
  Duration();
  explicit Duration(const struct timeval& t)
      : ns_(t.tv_sec * kSecond + t.tv_usec * kMicrosecond){};
  explicit Duration(int64_t nanoseconds) : ns_(nanoseconds){};
  explicit Duration(int nanoseconds) : ns_(nanoseconds){};
  explicit Duration(double seconds)
      : ns_(static_cast<int64_t>(seconds * kSecond)){};

  // Nanoseconds returns the duration as an integer nanosecond count.
  int64_t Nanoseconds() const { return ns_; };

  // These methods return double because the dominant
  // use case is for printing a floating point number like 1.5s, and
  // a truncation to integer would make them not useful in those cases.

  // Seconds returns the duration as a floating point number of seconds.
  double Seconds() const { return static_cast<double>(ns_) / kSecond; };

  double Milliseconds() const {
    return static_cast<double>(ns_) / kMillisecond;
  };
  double Microseconds() const {
    return static_cast<double>(ns_) / kMicrosecond;
  };
  double Minutes() const { return static_cast<double>(ns_) / kMinute; };
  double Hours() const { return static_cast<double>(ns_) / kHour; };

  struct timeval TimeVal() const {
    struct timeval t;
    To(&t);
    return t;
  };
  void To(struct timeval* t) const {
    t->tv_sec = (ns_ / kSecond);
    t->tv_usec = (ns_ % kSecond) / static_cast<long>(kMicrosecond);
  };

  bool IsZero() const { return ns_ == 0; };
  bool operator<(const Duration& rhs) const { return ns_ < rhs.ns_; };
  bool operator<=(const Duration& rhs) const { return ns_ <= rhs.ns_; };
  bool operator>(const Duration& rhs) const { return ns_ > rhs.ns_; };
  bool operator>=(const Duration& rhs) const { return ns_ >= rhs.ns_; };
  bool operator==(const Duration& rhs) const { return ns_ == rhs.ns_; };

  Duration operator+=(const Duration& rhs) {
    ns_ += rhs.ns_;
    return *this;
  };
  Duration operator-=(const Duration& rhs) {
    ns_ -= rhs.ns_;
    return *this;
  };
  Duration operator*=(int n) {
    ns_ *= n;
    return *this;
  };
  Duration operator/=(int n) {
    ns_ /= n;
    return *this;
  };

 private:
  int64_t ns_;  // nanoseconds
};
}  // namespace rnet::Unix