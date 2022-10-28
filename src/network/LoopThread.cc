#include "network/LoopThread.h"

#include <cassert>
#include <mutex>

#include "network/EventLoop.h"
namespace rnet::network {
// 二段式构造,初始化时io loop还没有生成,需要手动start
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const std::string& name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  //一般来说类不会正常析构,否则意味着程序退出
  if (loop_ != nullptr)  // not 100% race-free, eg. threadFunc could be running
                         // callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    loop_->quit();
    thread_.Join();
  }
}
EventLoop* EventLoopThread::startLoop() {
  assert(!thread_.Started());
  thread_.Start();

  // 这里可以用倒计时
  // 用条件变量是等待线程启动并完成loop的初始化,否则loop可能返回nullptr
  EventLoop* loop = nullptr;
  {
    std::unique_lock lock{mutex_};
    cond_.wait(lock, [this] { return loop_ != nullptr; });
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  {
    std::lock_guard lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  //开启事件循环等待io
  loop.loop();
  // 程序一般不会走到这里
  // assert(exiting_);
  std::lock_guard lock(mutex_);
  loop_ = nullptr;
  // ? set tLocalThread to nullptr
}

}  // namespace rnet::Network