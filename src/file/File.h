#pragma once
#include <sys/types.h>

#include <exception>
#include <string>
#include <string_view>

#include "base/Common.h"

namespace rnet::File {

// use stack
class FileReader {
 public:
  FileReader(std::string_view filename);
  DISALLOW_COPY(FileReader)
  ~FileReader();

  // return errno
  template <typename String>
  int readToString(int maxSize, String* content, int64_t* fileSize,
                   int64_t* modifyTime, int64_t* createTime);

  /// Read at maximum kBufferSize into buf_
  // return errno
  int readToBuffer(int* size);

  const char* buffer() const { return buf_; }
  // if size is set too  large
  // thread stack will overflow easily
  static const int kBufferSize = 16 * 1024;

  static std::string getFileContent(std::string_view file_name) noexcept(false);

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
  FileReader file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

class AppendFile {
 public:
  explicit AppendFile(std::string_view filename);

  ~AppendFile();

  void append(const char* buf, size_t len);

  void flush();

  off_t writtenBytes() const { return writtenBytes_; }

 private:
  size_t write(const char* logline, size_t len);

  FILE* fp_;
  char buffer_[64 * 1024];
  off_t writtenBytes_;
};

}  // namespace rnet::File