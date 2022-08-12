#include "unix/Duration.h"

namespace rnet::Unix {
constexpr int64_t Duration::kNanosecond = 1LL;
constexpr int64_t Duration::kMicrosecond = 1000 * kNanosecond;
constexpr int64_t Duration::kMillisecond = 1000 * kMicrosecond;
constexpr int64_t Duration::kSecond = 1000 * kMillisecond;
constexpr int64_t Duration::kMinute = 60 * kSecond;
constexpr int64_t Duration::kHour = 60 * kMinute;
}  // namespace rnet::Unix