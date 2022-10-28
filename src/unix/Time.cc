#include "Time.h"

#include <sys/time.h>

#include <cassert>
#include <cinttypes>
#include <vector>

using namespace rnet::Unix;
using namespace std;

static_assert( sizeof( Timestamp ) == sizeof( int64_t ), "Timestamp should be same size as int64_t" );

namespace rnet::detail {}  // namespace rnet::detail

std::string Timestamp::ToString() const {
  char    buf[ 32 ]    = { 0 };
  int64_t seconds      = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  snprintf( buf, sizeof( buf ), "%" PRId64 ".%06" PRId64 "", seconds, microseconds );
  return buf;
}

std::string Timestamp::ToFormattedString( bool showMicroseconds ) const {
  char      buf[ 64 ] = { 0 };
  auto      seconds   = static_cast< time_t >( microSecondsSinceEpoch_ / kMicroSecondsPerSecond );
  struct tm tmTime;
  gmtime_r( &seconds, &tmTime );

  if ( showMicroseconds ) {
    int microseconds = static_cast< int >( microSecondsSinceEpoch_ % kMicroSecondsPerSecond );
    snprintf( buf, sizeof( buf ), "%4d%02d%02d %02d:%02d:%02d.%06d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec, microseconds );
  }
  else {
    snprintf( buf, sizeof( buf ), "%4d%02d%02d %02d:%02d:%02d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec );
  }
  return buf;
}

Timestamp Timestamp::Now() {
  struct timeval tv;
  gettimeofday( &tv, nullptr );
  int64_t seconds = tv.tv_sec;
  return Timestamp( seconds * kMicroSecondsPerSecond + tv.tv_usec );
}
