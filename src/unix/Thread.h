#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "base/Common.h"
namespace rnet {

namespace Thread {
extern thread_local int tCachedThreadId;
extern thread_local char tTreadIdString[32];
extern thread_local int tTreadIdStringLen;
extern thread_local const char* tThreadName;

const char* getErrnoMessage(int savedErrno);
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

class CountDownLatch : noncopyable {
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

class Thread : noncopyable {
 public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc, const std::string& name = std::string());
  // FIXME: make it movable in C++11
  ~Thread();

  void start();
  void join();  // return pthread_join()

  bool started() const { return started_; }
  pid_t gettid() const { return tid_; }
  const std::string& name() const { return name_; }

  static int numCreated() {
    return numCreated_.load(std::memory_order_acq_rel);
  }

 private:
  void setDefaultName();

  bool started_;
  std::unique_ptr<std::thread> thread_;
  pid_t tid_;
  ThreadFunc func_;
  std::string name_;

  // 此倒计时用于
  CountDownLatch latch_;

  static std::atomic_int32_t numCreated_;
};

}  // namespace Thread

// TODO string stackTrace(bool demangle)

}  // namespace rnet
