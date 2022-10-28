#pragma once

#include "base/Common.h"
#include "file/Buffer.h"
#include <cmath>
namespace rnet::log {

class LogStream : Noncopyable {
  using self = LogStream;

public:
  using Buffer = file::SizedBuffer< file::kSmallSize >;

  self& operator<<( bool v ) {
    buffer_.Append( v ? "1" : "0", 1 );
    return *this;
  }

  self& operator<<( int16_t );
  self& operator<<( uint16_t );
  self& operator<<( int32_t );
  self& operator<<( uint32_t );
  self& operator<<( int64_t );
  self& operator<<( uint64_t );

  self& operator<<( const void* );

  self& operator<<( float v ) {
    *this << static_cast< double >( v );
    return *this;
  }
  self& operator<<( double );
  // self& operator<<(long double);

  self& operator<<( char v ) {
    buffer_.Append( &v, 1 );
    return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<( const char* str ) {
    if ( str != nullptr ) {
      buffer_.Append( str, strlen( str ) );
    }
    else {
      buffer_.Append( "(null)", 6 );  // NOLINT
    }
    return *this;
  }

  self& operator<<( const unsigned char* str ) {
    return operator<<( reinterpret_cast< const char* >( str ) );  // NOLINT
  }

  self& operator<<( const std::string& v ) {
    buffer_.Append( v.c_str(), v.size() );
    return *this;
  }

  self& operator<<( std::string_view v ) {
    buffer_.Append( v.data(), v.size() );
    return *this;
  }

  self& operator<<( const Buffer& /*v*/ ) {
    return *this;
  }

  void Append( const char* data, int len ) {
    buffer_.Append( data, len );
  }
  const Buffer& GetBuffer() const {
    return buffer_;
  }
  void ResetBuffer() {
    buffer_.Reset();
  }

private:
  void StaticCheck();

  template < typename T > void FormatInteger( T );

  Buffer buffer_;

  static const int kMaxNumericSize = 48;
};
}  // namespace rnet::log