#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "Common.h"
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



class CountDownLatch : detail::noncopyable {
 public:
  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  int count_;
};

}  // namespace Thread

// TODO string stackTrace(bool demangle)



}  // namespace rnet
