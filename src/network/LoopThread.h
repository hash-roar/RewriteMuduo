#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>

#include "base/Common.h"
#include "unix/Thread.h"
namespace rnet::network {
class EventLoop;

// one loop per thread.
// 作为io线程,由主线程接受连接后分发给io线程,做后续的io操作
// 本类为一个io线程的抽象
// 线程退出基本意味着程序结束,没有对io loop做其余的清理操作
class EventLoopThread : Noncopyable {
 public:
  // 线程初始化时的回调函数,由tcp server注册到thread pool再注册到thread
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const std::string& name = std::string{});
  ~EventLoopThread();
  EventLoop* StartLoop();

 private:
  // 线程运行的函数,就是io loop的大循环
  void ThreadFunc();

  EventLoop* loop_;
  bool exiting_;
  thread::Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};
}  // namespace rnet::Network