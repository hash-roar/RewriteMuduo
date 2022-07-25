#pragma once

// 一些最基础的组件
#include <cstddef>
#include <cstring>
namespace rnet::detail {

// 直接继承此类导致子类不可被复制
class noncopyable {
 public:
  noncopyable(const noncopyable &) = delete;
  void operator=(const noncopyable &) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

// 显式表明隐式转换
template <typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}
// 将缓冲区置零
inline void memZero(void *p, size_t n) { memset(p, 0, n); }

}  // namespace rnet::detail