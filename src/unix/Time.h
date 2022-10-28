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
  Timestamp() : microSecondsSinceEpoch_( 0 ) {}

  ///
  /// Constructs a Timestamp at specific time
  ///
  /// @param microSecondsSinceEpoch
  explicit Timestamp( int64_t microSecondsSinceEpochArg ) : microSecondsSinceEpoch_( microSecondsSinceEpochArg ) {}

  void Swap( Timestamp& that ) {
    std::swap( microSecondsSinceEpoch_, that.microSecondsSinceEpoch_ );
  }

  // default copy/assignment/dtor are Okay

  std::string ToString() const;
  std::string ToFormattedString( bool showMicroseconds = true ) const;

  bool Valid() const {
    return microSecondsSinceEpoch_ > 0;
  }

  SysTimep_t ToChronoSysTime() const {
    return SysTimep_t{ std::chrono::microseconds{ microSecondsSinceEpoch_ } };
  }
  // for internal usage.
  int64_t MicroSecondsSinceEpoch() const {
    return microSecondsSinceEpoch_;
  }
  time_t SecondsSinceEpoch() const {
    return static_cast< time_t >( microSecondsSinceEpoch_ / kMicroSecondsPerSecond );
  }

  ///
  /// Get time of now.
  ///
  static Timestamp Now();
  static Timestamp Invalid() {
    return Timestamp();
  }

  static Timestamp FromUnixTime( time_t t ) {
    return FromUnixTime( t, 0 );
  }

  static Timestamp FromUnixTime( time_t t, int microseconds ) {
    return Timestamp( static_cast< int64_t >( t ) * kMicroSecondsPerSecond + microseconds );
  }

  static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<( Timestamp lhs, Timestamp rhs ) {
  return lhs.MicroSecondsSinceEpoch() < rhs.MicroSecondsSinceEpoch();
}

inline bool operator==( Timestamp lhs, Timestamp rhs ) {
  return lhs.MicroSecondsSinceEpoch() == rhs.MicroSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double TimeDifference( Timestamp high, Timestamp low ) {
  int64_t diff = high.MicroSecondsSinceEpoch() - low.MicroSecondsSinceEpoch();
  return static_cast< double >( diff ) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp AddTime( Timestamp timestamp, double seconds ) {
  auto delta = static_cast< int64_t >( seconds * Timestamp::kMicroSecondsPerSecond );
  return Timestamp( timestamp.MicroSecondsSinceEpoch() + delta );
}

}  // namespace rnet::Unix