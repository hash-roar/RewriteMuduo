#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/Common.h"
namespace rnet::network {
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : Noncopyable {
 public:
  //事件回调,注册到loop thread中
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
  ~EventLoopThreadPool();
  // 设置io线程数量,默认为0 即io都在主线程做
  void SetThreadNum(int numThreads) { numThreads_ = numThreads; }
  void Start(const ThreadInitCallback& cb = ThreadInitCallback());

  // 暴露给tcp server用于分发tcp connection任务,
  // 在start调用后才能调用,否则断言失败程序崩溃
  EventLoop* GetNextLoop();

  // 简单对hash code 取余选取loop
  EventLoop* GetLoopForHash(size_t hashCode);

  std::vector<EventLoop*> GetAllLoops();

  bool Started() const { return started_; }

  const std::string& Name() const { return name_; }

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