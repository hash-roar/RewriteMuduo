#include "file/Buffer.h"

#include <algorithm>
#include <cinttypes>

#include "file/File.h"

namespace rnet::file {

template < int SIZE > const char* SizedBuffer< SIZE >::DebugString() {
    *cur_ = '\0';
    return data_;
}
}  // namespace rnet::file