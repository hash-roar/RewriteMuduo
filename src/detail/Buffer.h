#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "Common.h"

namespace rnet::detail {

constexpr int kLargeSize = 1024 * 4;
constexpr int kSmallSize = 1024 * 1000;

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

// 对buffer的包装,重载<<运算符使得支持流式语法
class LogStream : noncopyable {
  using self = LogStream;

 public:
  using Buffer = SizedBuffer<kSmallSize>;

  self& operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  self& operator<<(int16_t);
  self& operator<<(uint16_t);
  self& operator<<(int32_t);
  self& operator<<(uint32_t);
  self& operator<<(int64_t);
  self& operator<<(uint64_t);

  self& operator<<(const void*);

  self& operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }
  self& operator<<(double);
  // self& operator<<(long double);

  self& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<(const char* str) {
    if (str != nullptr) {
      buffer_.append(str, strlen(str));
    } else {
      buffer_.append("(null)", 6);
    }
    return *this;
  }

  self& operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  self& operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  self& operator<<(std::string_view v) {
    buffer_.append(v.data(), v.size());
    return *this;
  }

  self& operator<<(const Buffer& v) { return *this; }

  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:
  void staticCheck();

  template <typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 48;
};

}  // namespace rnet::detail