#include "log/LogStream.h"

#include <algorithm>
#include <cinttypes>

#include "file/Buffer.h"

namespace rnet::log {
const char        digits[] = "9876543210123456789";
const char* const zero     = digits + 9;
static_assert( sizeof( digits ) == 20, "wrong number of digits" );

const char digitsHex[] = "0123456789ABCDEF";
static_assert( sizeof digitsHex == 17, "wrong number of digitsHex" );
// Efficient Integer to String Conversions, by Matthew Wilson.
template < typename T > size_t Convert( char buf[], T value ) {
  T     i = value;
  char* p = buf;

  do {
    int lsd = static_cast< int >( i % 10 );
    i /= 10;
    *p++ = zero[ lsd ];
  } while ( i != 0 );

  if ( value < 0 ) {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse( buf, p );

  return p - buf;
}
size_t ConvertHex( char buf[], uintptr_t value ) {
  uintptr_t i = value;
  char*     p = buf;

  do {
    int lsd = static_cast< int >( i % 16 );
    i /= 16;
    *p++ = digitsHex[ lsd ];
  } while ( i != 0 );

  *p = '\0';
  std::reverse( buf, p );

  return p - buf;
}

std::string FormatSi( int64_t s ) {
  auto n = static_cast< double >( s );
  char buf[ 64 ];
  if ( s < 1000 ) {
    snprintf( buf, sizeof( buf ), "%" PRId64, s );
  }
  else if ( s < 9995 ) {
    snprintf( buf, sizeof( buf ), "%.2fk", n / 1e3 );
  }
  else if ( s < 99950 ) {
    snprintf( buf, sizeof( buf ), "%.1fk", n / 1e3 );
  }
  else if ( s < 999500 ) {
    snprintf( buf, sizeof( buf ), "%.0fk", n / 1e3 );
  }
  else if ( s < 9995000 ) {
    snprintf( buf, sizeof( buf ), "%.2fM", n / 1e6 );
  }
  else if ( s < 99950000 ) {
    snprintf( buf, sizeof( buf ), "%.1fM", n / 1e6 );
  }
  else if ( s < 999500000 ) {
    snprintf( buf, sizeof( buf ), "%.0fM", n / 1e6 );
  }
  else if ( s < 9995000000 ) {
    snprintf( buf, sizeof( buf ), "%.2fG", n / 1e9 );
  }
  else if ( s < 99950000000 ) {
    snprintf( buf, sizeof( buf ), "%.1fG", n / 1e9 );
  }
  else if ( s < 999500000000 ) {
    snprintf( buf, sizeof( buf ), "%.0fG", n / 1e9 );
  }
  else if ( s < 9995000000000 ) {
    snprintf( buf, sizeof( buf ), "%.2fT", n / 1e12 );
  }
  else if ( s < 99950000000000 ) {
    snprintf( buf, sizeof( buf ), "%.1fT", n / 1e12 );
  }
  else if ( s < 999500000000000 ) {
    snprintf( buf, sizeof( buf ), "%.0fT", n / 1e12 );
  }
  else if ( s < 9995000000000000 ) {
    snprintf( buf, sizeof( buf ), "%.2fP", n / 1e15 );
  }
  else if ( s < 99950000000000000 ) {
    snprintf( buf, sizeof( buf ), "%.1fP", n / 1e15 );
  }
  else if ( s < 999500000000000000 ) {
    snprintf( buf, sizeof( buf ), "%.0fP", n / 1e15 );
  }
  else {
    snprintf( buf, sizeof( buf ), "%.2fE", n / 1e18 );
  }
  return buf;
}

std::string FormatIec( int64_t s ) {
  double       n = static_cast< double >( s );
  char         buf[ 64 ];
  const double ki = 1024.0;
  const double mi = ki * 1024.0;
  const double gi = mi * 1024.0;
  const double ti = gi * 1024.0;
  const double pi = ti * 1024.0;
  const double ei = pi * 1024.0;

  if ( n < ki ) {
    snprintf( buf, sizeof buf, "%" PRId64, s );
  }
  else if ( n < ki * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fKi", n / ki );
  }
  else if ( n < ki * 99.95 ) {
    snprintf( buf, sizeof buf, "%.1fKi", n / ki );
  }
  else if ( n < ki * 1023.5 ) {
    snprintf( buf, sizeof buf, "%.0fKi", n / ki );
  }
  else if ( n < mi * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fMi", n / mi );
  }
  else if ( n < mi * 99.95 ) {
    snprintf( buf, sizeof buf, "%.1fMi", n / mi );
  }
  else if ( n < mi * 1023.5 ) {
    snprintf( buf, sizeof buf, "%.0fMi", n / mi );
  }
  else if ( n < gi * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fGi", n / gi );
  }
  else if ( n < gi * 99.95 ) {
    snprintf( buf, sizeof buf, "%.1fGi", n / gi );
  }
  else if ( n < gi * 1023.5 ) {
    snprintf( buf, sizeof buf, "%.0fGi", n / gi );
  }
  else if ( n < ti * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fTi", n / ti );
  }
  else if ( n < ti * 99.95 ) {
    snprintf( buf, sizeof buf, "%.1fTi", n / ti );
  }
  else if ( n < ti * 1023.5 ) {
    snprintf( buf, sizeof buf, "%.0fTi", n / ti );
  }
  else if ( n < pi * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fPi", n / pi );
  }
  else if ( n < pi * 99.95 ) {
    snprintf( buf, sizeof buf, "%.1fPi", n / pi );
  }
  else if ( n < pi * 1023.5 ) {
    snprintf( buf, sizeof buf, "%.0fPi", n / pi );
  }
  else if ( n < ei * 9.995 ) {
    snprintf( buf, sizeof buf, "%.2fEi", n / ei );
  }
  else {
    snprintf( buf, sizeof buf, "%.1fEi", n / ei );
  }
  return buf;
}

void LogStream::StaticCheck() {
  static_assert( kMaxNumericSize - 10 > std::numeric_limits< double >::digits10, "kMaxNumericSize is large enough" );
  static_assert( kMaxNumericSize - 10 > std::numeric_limits< long double >::digits10, "kMaxNumericSize is large enough" );
  static_assert( kMaxNumericSize - 10 > std::numeric_limits< int64_t >::digits10, "kMaxNumericSize is large enough" );
  static_assert( kMaxNumericSize - 10 > std::numeric_limits< int64_t >::digits10, "kMaxNumericSize is large enough" );
}

template < typename T > void LogStream::FormatInteger( T v ) {
  if ( buffer_.Avail() >= kMaxNumericSize ) {
    size_t len = Convert( buffer_.Current(), v );
    buffer_.Add( len );
  }
}

LogStream& LogStream::operator<<( int16_t v ) {
  *this << static_cast< int >( v );
  return *this;
}

LogStream& LogStream::operator<<( uint16_t v ) {
  *this << static_cast< unsigned int >( v );
  return *this;
}

LogStream& LogStream::operator<<( int32_t v ) {
  FormatInteger( v );
  return *this;
}

LogStream& LogStream::operator<<( uint32_t v ) {
  FormatInteger( v );
  return *this;
}

LogStream& LogStream::operator<<( int64_t v ) {
  FormatInteger( v );
  return *this;
}

LogStream& LogStream::operator<<( uint64_t v ) {
  FormatInteger( v );
  return *this;
}

LogStream& LogStream::operator<<( const void* p ) {
  uintptr_t v = reinterpret_cast< uintptr_t >( p );
  if ( buffer_.Avail() >= kMaxNumericSize ) {
    char* buf  = buffer_.Current();
    buf[ 0 ]   = '0';
    buf[ 1 ]   = 'x';
    size_t len = ConvertHex( buf + 2, v );
    buffer_.Add( len + 2 );
  }
  return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<( double v ) {
  if ( buffer_.Avail() >= kMaxNumericSize ) {
    int len = snprintf( buffer_.Current(), kMaxNumericSize, "%.12g", v );
    buffer_.Add( len );
  }
  return *this;
}

}  // namespace rnet::log
