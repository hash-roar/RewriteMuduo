#include "FileOpt.h"
#include <cstddef>
#include <filesystem>

namespace rnet::file {
size_t FileHelper::FileSize( std::string_view path ) {
  namespace fs = std::filesystem;
  return fs::file_size( fs::path{ path } );
}
}  // namespace rnet::file