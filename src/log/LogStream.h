#pragma once

#include <cmath>
#include "base/Common.h"
#include "file/Buffer.h"
namespace rnet::log {

class LogStream : noncopyable {
  using self = LogStream;

 public:
  using Buffer = File::SizedBuffer<File::kSmallSize>;

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
}  // namespace rnet::log