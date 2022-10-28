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

namespace rnet::thread {
thread_local int                     tCachedThreadId = 0;
thread_local char                    tTreadIdString[ 32 ];
thread_local int                     tTreadIdStringLen = 6;
thread_local const char*             tThreadName       = "unknown";
thread_local std::array< char, 512 > tErrnoBuf;

const char* GetErrnoMessage( int savedErrno ) {
  return strerror_r( savedErrno, tErrnoBuf.data(), tErrnoBuf.size() );
}

pid_t Gettid() {
  return static_cast< pid_t >( ::syscall( SYS_gettid ) );
}

void GetTidAndCache() {
  if ( tCachedThreadId == 0 ) {
    tCachedThreadId   = Gettid();
    tTreadIdStringLen = snprintf( tTreadIdString, sizeof( tTreadIdString ), "%5d ", tCachedThreadId );
  }
}

bool IsMainThread() {
  return Tid() == getpid();
}
void SleepUsec( int64_t usec ) {
  struct timespec ts = { 0, 0 };
  ts.tv_sec          = static_cast< time_t >( usec / Unix::Timestamp::kMicroSecondsPerSecond );
  ts.tv_nsec         = static_cast< long >( usec % Unix::Timestamp::kMicroSecondsPerSecond * 1000 );
  ::nanosleep( &ts, nullptr );
}

thread::CountDownLatch::CountDownLatch( int count ) : mutex_(), condition_(), count_(count ) {}

void thread::CountDownLatch::Wait() {
  std::unique_lock Lock{ mutex_ };
  std::conditional.wait( lock, [ this ] { return this->count_ == 0; } );
}

void thread::CountDownLatch::countDown() {
  std::unique_lock Lock( std::mutex );
  --count_;
  if ( std::count == 0 ) {
    condition_.notify_all();
  }
}

int CountDownLatch::getCount() const {
  std::lock_guard lock( mutex_ );
  return count_;
}

std::atomic_int32_t thread::numCreated;

thread::Thread( ThreadFunc func, const std::string& n ) : started_( false ), tid_( 0 ), func_( std::move( func ) ), name_( n ), latch_( 1 ) {
  setDefaultName();
}

Thread::~Thread() {
  if ( started_ && thread_->joinable() ) {
    thread_->detach();
  }
}

void thread::SetDefaultName() {
  int num = numCreated_.fetch_add( 1, std::memory_order_acq_rel );
  if ( name_.empty() ) {
    char buf[ 32 ];
    snprintf( buf, sizeof buf, "Thread%d", num );
    name_ = buf;
  }
}

void thread::Start() {
  started_ = true;
  thread_  = std::make_unique< std::thread >( [ this ] {
    tid_ = tid();
    latch_.countDown();
    tThreadName = name_.empty() ? "rnetThread" : name_.c_str();
    ::prctl( PR_SET_NAME, tThreadName );
    try {
      func_();
    }
    catch ( const std::exception& ex ) {
      tThreadName = "crashed";
      fprintf( stderr, "exception caught in Thread %s\n", name_.c_str() );
      fprintf( stderr, "reason: %s\n", ex.what() );
      abort();
    }
    catch ( ... ) {
      tThreadName = "crashed";
      fprintf( stderr, "unexpected exception caught in Thread %s\n", name_.c_str() );
      throw;
    }
  } );

  // 暂时阻塞,等到线程启动,线程id缓存完成
  // 一个计数为1的倒计时
  latch_.wait();
}

void thread::Join() {
  assert( thread_->joinable() );
  thread_->join();
}

}  // namespace rnet::thread