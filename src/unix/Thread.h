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

namespace thread {
  extern thread_local int         tCachedThreadId;
  extern thread_local char        tTreadIdString[ 32 ];
  extern thread_local int         tTreadIdStringLen;
  extern thread_local const char* tThreadName;

  const char* GetErrnoMessage( int savedErrno );
  void        GetTidAndCache();

  inline int Tid() {
    if ( tCachedThreadId == 0 ) {
      GetTidAndCache();
    }
    return tCachedThreadId;
  }

  inline const char* TidString()  // for logging
  {
    return tTreadIdString;
  }

  inline int TidStringLength()  // for logging
  {
    return tTreadIdStringLen;
  }

  inline const char* Name() {
    return tThreadName;
  }

  bool IsMainThread();

  void SleepUsec( int64_t usec );  // for testing

  class CountDownLatch : Noncopyable {
  public:
    explicit CountDownLatch( int count );

    void Wait();

    void CountDown();

    int GetCount() const;

  private:
    mutable std::mutex      mutex_;
    std::condition_variable condition_;
    int                     count_;
  };

  class Thread : Noncopyable {
  public:
    using ThreadFunc = std::function< void() >;

    explicit Thread( ThreadFunc, const std::string& name = std::string() );
    // FIXME: make it movable in C++11
    ~Thread();

    void Start();
    void Join();  // return pthread_join()

    bool Started() const {
      return started_;
    }
    pid_t Gettid() const {
      return tid_;
    }
    const std::string& Name() const {
      return name_;
    }

    static int NumCreated() {
      return numCreated.load( std::memory_order_acq_rel );
    }

  private:
    void SetDefaultName();

    bool                           started_;
    std::unique_ptr< std::thread > thread_;
    pid_t                          tid_;
    ThreadFunc                     func_;
    std::string                    name_;

    // 此倒计时用于
    CountDownLatch latch_;

    static std::atomic_int32_t numCreated;
  };

}  // namespace thread

// TODO: string stackTrace(bool demangle) .use c++23 library

}  // namespace rnet
