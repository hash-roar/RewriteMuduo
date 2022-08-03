#pragma once

#include <cstddef>
#include <string_view>
namespace rnet::File {
class FileHelper {
 public:
  FileHelper();
  ~FileHelper();
  enum FsError {
    NOT_FOUND,
    PERMISSION,
    NON_REGULAR_FILE,
    NONE,
  };
  static size_t fileSize(std::string_view path);

 private:
};
}  // namespace rnet::File