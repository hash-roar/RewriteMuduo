#pragma once
#include <sys/types.h>

#include <string_view>

#include "base/Common.h"

namespace rnet::File {

class ReadSmallFile : noncopyable {
 public:
  ReadSmallFile(std::string_view filename);
  ~ReadSmallFile();

  // return errno
  template <typename String>
  int readToString(int maxSize, String* content, int64_t* fileSize,
                   int64_t* modifyTime, int64_t* createTime);

  /// Read at maxium kBufferSize into buf_
  // return errno
  int readToBuffer(int* size);

  const char* buffer() const { return buf_; }

  static const int kBufferSize = 64 * 1024;

 private:
  int fd_;
  int err_;
  char buf_[kBufferSize];
};

// read the file content, returns errno if error happens.
template <typename String>
int readFile(std::string_view filename, int maxSize, String* content,
             int64_t* fileSize = nullptr, int64_t* modifyTime = nullptr,
             int64_t* createTime = nullptr) {
  ReadSmallFile file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

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

}  // namespace rnet::File