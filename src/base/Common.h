#pragma once

// 一些最基础的组件
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace rnet {

constexpr size_t kbSize = 1024;

using SysTimep_t    = std::chrono::time_point< std::chrono::system_clock >;
using SteadyTimep_t = std::chrono::time_point< std::chrono::steady_clock >;
// 直接继承此类导致子类不可被复制
class Noncopyable {
public:
  Noncopyable( const Noncopyable& )    = delete;
  void operator=( const Noncopyable& ) = delete;

protected:
  Noncopyable()  = default;
  ~Noncopyable() = default;
};
class Copyable {
protected:
  Copyable()  = default;
  ~Copyable() = default;
};

// 显式表明隐式转换
template < typename To, typename From >
inline To implicit_cast( From const& f )  // NOLINT: same style with static_cast
{
  return f;
}
// 将缓冲区置零
inline void MemZero( void* p, size_t n ) {
  memset( p, 0, n );
}

#define RNET_ASSERT( expr, message ) assert( ( expr ) && ( message ) )

#define UNREACHABLE( message ) throw std::logic_error( message )

// Macros to disable copying and moving
#define DISALLOW_COPY( cname )                            \
  cname( const cname& )            = delete; /* NOLINT */ \
  cname& operator=( const cname& ) = delete; /* NOLINT */

#define DISALLOW_MOVE( cname )                       \
  cname( cname&& )            = delete; /* NOLINT */ \
  cname& operator=( cname&& ) = delete; /* NOLINT */

#define DISALLOW_COPY_AND_MOVE( cname ) \
  DISALLOW_COPY( cname );               \
  DISALLOW_MOVE( cname );

}  // namespace rnet