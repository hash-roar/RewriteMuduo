#pragma once

#include <cstdint>
namespace rnet {

namespace Thread {
extern thread_local int tCachedThreadId;
extern thread_local char tTreadIdString[32];
extern thread_local int tTreadIdStringLen;
extern thread_local const char* tThreadName;

void getTidAndCache();

inline int tid() {
  if (tCachedThreadId == 0) {
    getTidAndCache();
  }
  return tCachedThreadId;
}

inline const char* tidString()  // for logging
{
  return tTreadIdString;
}

inline int tidStringLength()  // for logging
{
  return tTreadIdStringLen;
}

inline const char* name() { return tThreadName; }

bool isMainThread();

void sleepUsec(int64_t usec);  // for testing
}  // namespace Thread

// TODO string stackTrace(bool demangle)

}  // namespace rnet
