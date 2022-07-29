#pragma once
#include <sys/types.h>
#include <string_view>

#include "base/Common.h"

namespace rnet::File {

class AppendFile : noncopyable {
 public:
  explicit AppendFile(std::string_view filename);

  ~AppendFile();

  void append(const char* logline, size_t len);

  void flush();

  off_t writtenBytes() const { return writtenBytes_; }

 private:
  size_t write(const char* logline, size_t len);

  FILE* fp_;
  char buffer_[64 * 1024];
  off_t writtenBytes_;
};
}  // namespace rnet::detail