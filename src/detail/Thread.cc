#include "Thread.h"

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <mutex>

#include "Time.h"

using namespace rnet;

namespace rnet::Thread {
thread_local int tCachedThreadId = 0;
thread_local char tTreadIdString[32];
thread_local int tTreadIdStringLen = 6;
thread_local const char* tThreadName = "unknown";

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void getTidAndCache() {
  if (tCachedThreadId == 0) {
    tCachedThreadId = gettid();
    tTreadIdStringLen = snprintf(tTreadIdString, sizeof(tTreadIdString), "%5d ",
                                 tCachedThreadId);
  }
}

bool isMainThread() { return tid() == getpid(); }
void sleepUsec(int64_t usec) {
  struct timespec ts = {0, 0};
  ts.tv_sec =
      static_cast<time_t>(usec / detail::Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      usec % detail::Timestamp::kMicroSecondsPerSecond * 1000);
  ::nanosleep(&ts, NULL);
}

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(), count_(count) {}

void CountDownLatch::wait() {
  std::unique_lock lock(mutex_);
  condition_.wait(lock, [this] { return this->count_ > 0; });
}

void CountDownLatch::countDown() {
  std::unique_lock lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notify_all();
  }
}

int CountDownLatch::getCount() const {
  std::lock_guard lock(mutex_);
  return count_;
}

}  // namespace rnet::Thread