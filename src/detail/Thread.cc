#include "Thread.h"

#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

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
  condition_.wait(lock, [this] { return this->count_ == 0; });
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

std::atomic_int32_t Thread::numCreated_;

Thread::Thread(ThreadFunc func, const std::string& n)
    : started_(false), tid_(0), func_(std::move(func)), name_(n), latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && thread_->joinable()) {
    thread_->detach();
  }
}

void Thread::setDefaultName() {
  int num = numCreated_.fetch_add(1, std::memory_order_acq_rel);
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", num);
    name_ = buf;
  }
}

void Thread::start() {
  // assert(!started_);
  // started_ = true;
  // // FIXME: move(func_)
  // detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_,
  // &latch_); if (pthread_create(&pthreadId_, NULL, &detail::startThread,
  // data))
  // {
  //   started_ = false;
  //   delete data; // or no delete?
  //   LOG_SYSFATAL << "Failed in pthread_create";
  // }
  // else
  // {
  //   latch_.wait();
  //   assert(tid_ > 0);
  // }
  started_ = true;
  thread_ = std::make_unique<std::thread>([this] {
    tid_ = tid();
    latch_.countDown();
    tThreadName = name_.empty() ? "rnetThread" : name_.c_str();
    ::prctl(PR_SET_NAME, tThreadName);
    try {
      func_();
    } catch (const std::exception& ex) {
      tThreadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      abort();
    } catch (...) {
      tThreadName = "crashed";
      fprintf(stderr, "unexpected exception caught in Thread %s\n",
              name_.c_str());
      throw;
    }
  });

  latch_.wait();
}

void Thread::join() {
  assert(thread_->joinable());
  thread_->join();
}

}  // namespace rnet::Thread