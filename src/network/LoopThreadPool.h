#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/Common.h"
namespace rnet::Network {
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable {
 public:
  //事件回调,注册到loop thread中
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
  ~EventLoopThreadPool();
  // 设置io线程数量,默认为0 即io都在主线程做
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // 暴露给tcp server用于分发tcp connection任务,
  // 在start调用后才能调用,否则断言失败程序崩溃
  EventLoop* getNextLoop();

  // 简单对hash code 取余选取loop
  EventLoop* getLoopForHash(size_t hashCode);

  std::vector<EventLoop*> getAllLoops();

  bool started() const { return started_; }

  const std::string& name() const { return name_; }

 private:
  EventLoop* baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};

}  // namespace rnet::Network