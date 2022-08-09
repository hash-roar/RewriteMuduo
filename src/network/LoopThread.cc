#include "network/LoopThread.h"

#include <cassert>
#include <mutex>

#include "network/EventLoop.h"
namespace rnet::Network {
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
  if (loop_ != nullptr)  // not 100% race-free, eg. threadFunc could be running
                         // callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    loop_->quit();
    thread_.join();
  }
}
EventLoop* EventLoopThread::startLoop() {
  assert(!thread_.started());
  thread_.start();

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

  loop.loop();
  // assert(exiting_);
  std::lock_guard lock(mutex_);
  loop_ = nullptr;
  // ? set tLocalThread to nullptr
}

}  // namespace rnet::Network