#pragma once

#include <cstddef>
#include <string_view>
namespace rnet::file {
class FileHelper {
 public:
  FileHelper();
  ~FileHelper();
  enum FsError {
    notFound,
    permission,
    nonRegularFile,
    none,
  };
  static size_t FileSize(std::string_view path);

 private:
};
}  // namespace rnet::File