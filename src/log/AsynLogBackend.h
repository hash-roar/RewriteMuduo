#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "base/Common.h"
#include "file/Buffer.h"
#include "unix/Thread.h"

namespace rnet::log {
class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string& basename, off_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging() {
    if (running_) {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify_one();
    thread_.join();
  }

 private:
  void threadFunc();

  using Buffer = rnet::File::SizedBuffer<rnet::File::kLargeSize>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  const int flushInterval_;
  std::atomic<bool> running_{false};
  const std::string basename_;
  const off_t rollSize_;
  rnet::Thread::Thread thread_;
  rnet::Thread::CountDownLatch latch_;
  std::mutex mutex_;
  std::condition_variable cond_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
};
}  // namespace rnet::log