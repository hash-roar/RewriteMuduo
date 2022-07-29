#include "file/Buffer.h"

#include <algorithm>
#include <cinttypes>

#include "file/File.h"

using namespace rnet::File;

namespace rnet::File {


template <int SIZE>
const char* SizedBuffer<SIZE>::debugString() {
  *cur_ = '\0';
  return data_;
}
}  // namespace rnet::File