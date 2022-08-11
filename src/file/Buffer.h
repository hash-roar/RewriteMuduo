#pragma once

#include <string>

#include "base/Common.h"
namespace rnet::File {
constexpr int kSmallSize = 1024 * 4;
constexpr int kLargeSize = 1024 * 1000;

// 对数组的简单包装,缓冲区在对象内
template <int SIZE>
class SizedBuffer : noncopyable {
 public:
  SizedBuffer() : cur_(data_) {}

  ~SizedBuffer() = default;

  void append(const char* /*restrict*/ buf, size_t len) {
    // FIXME: append partially
    if (implicit_cast<size_t>(avail()) > len) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  // write to data_ directly
  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { memZero(data_, sizeof data_); }

  // for used by GDB
  const char* debugString();
  // for used by unit test
  std::string toString() const { return string(data_, length()); }
  std::string_view toStrView() const {
    return std::string_view(data_, length());
  }

 private:
  const char* end() const { return data_ + sizeof data_; }
  char data_[SIZE]{};
  char* cur_;
};

}  // namespace rnet::File