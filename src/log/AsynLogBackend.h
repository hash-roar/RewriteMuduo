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
class AsyncLogging : Noncopyable {
public:
  AsyncLogging( const std::string& basename, off_t rollSize, int flushInterval = 3 );

  ~AsyncLogging() {
    if ( running_ ) {
      Stop();
    }
  }

  void Append( const char* logline, int len );

  void Start() {
    running_ = true;
    thread_.Start();
    latch_.Wait();
  }

  void Stop() {
    running_ = false;
    cond_.notify_one();
    thread_.Join();
  }

private:
  void ThreadFunction();

  using Buffer       = rnet::file::SizedBuffer< rnet::file::kLargeSize >;
  using BufferVector = std::vector< std::unique_ptr< Buffer > >;
  using BufferPtr    = BufferVector::value_type;

  const int                    flushInterval;
  std::atomic< bool >          running_{ false };
  const std::string            basename;
  const off_t                  rollSize;
  rnet::thread::Thread         thread_;
  rnet::thread::CountDownLatch latch_;
  std::mutex                   mutex_;
  std::condition_variable      cond_;
  BufferPtr                    currentBuffer_;
  BufferPtr                    nextBuffer_;
  BufferVector                 buffers_;
};
}  // namespace rnet::log