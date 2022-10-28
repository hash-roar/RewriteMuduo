#pragma once

#include <string>

#include "base/Common.h"
namespace rnet::file {
constexpr int kSmallSize = 1024 * 4;
constexpr int kLargeSize = 1024 * 1000;

// 对数组的简单包装,缓冲区在对象内
template < int SIZE > class SizedBuffer : Noncopyable {
public:
  SizedBuffer() : cur_( data_ ) {}

  ~SizedBuffer() = default;

  void Append( const char* /*restrict*/ buf, size_t len ) {
    // FIXME: append partially
    if ( implicit_cast< size_t >( Avail() ) > len ) {
      memcpy( cur_, buf, len );
      cur_ += len;
    }
  }

  const char* Data() const {
    return data_;
  }
  int Length() const {
    return static_cast< int >( cur_ - data_ );
  }

  // write to data_ directly
  char* Current() {
    return cur_;
  }
  int Avail() const {
    return static_cast< int >( End() - cur_ );
  }
  void Add( size_t len ) {
    cur_ += len;
  }

  void Reset() {
    cur_ = data_;
  }
  void Bzero() {
    MemZero( data_, sizeof data_ );
  }

  // for used by GDB
  const char* DebugString();
  // for used by unit test
  std::string ToString() const {
    return string( data_, Length() );
  }
  std::string_view ToStrView() const {
    return std::string_view( data_, Length() );
  }

private:
  const char* End() const {
    return data_ + sizeof data_;
  }
  char  data_[ SIZE ]{};
  char* cur_;
};

}  // namespace rnet::file