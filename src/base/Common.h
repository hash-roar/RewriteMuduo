#pragma once

// 一些最基础的组件
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace rnet {

using SysTimep_t = std::chrono::time_point<std::chrono::system_clock>;
using SteadyTimep_t = std::chrono::time_point<std::chrono::steady_clock>;
// 直接继承此类导致子类不可被复制
class noncopyable {
 public:
  noncopyable(const noncopyable &) = delete;
  void operator=(const noncopyable &) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};
class copyable {
 protected:
  copyable() = default;
  ~copyable() = default;
};

// 显式表明隐式转换
template <typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}
// 将缓冲区置零
inline void memZero(void *p, size_t n) { memset(p, 0, n); }

#pragma once

#define RNET_ASSERT(expr, message) assert((expr) && (message))

#define UNREACHABLE(message) throw std::logic_error(message)

// Macros to disable copying and moving
#define DISALLOW_COPY(cname)                             \
  cname(const cname &) = delete;            /* NOLINT */ \
  cname &operator=(const cname &) = delete; /* NOLINT */

#define DISALLOW_MOVE(cname)                        \
  cname(cname &&) = delete;            /* NOLINT */ \
  cname &operator=(cname &&) = delete; /* NOLINT */

#define DISALLOW_COPY_AND_MOVE(cname) \
  DISALLOW_COPY(cname);               \
  DISALLOW_MOVE(cname);

}  // namespace rnet