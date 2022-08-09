#include "network/EventLoop.h"

#include <sys/eventfd.h>

#include <csignal>

#include "log/Logger.h"
#include "network/Epoll.h"
#include "network/TimerQueue.h"
#include "unix/Thread.h"

using namespace rnet;
namespace {
thread_local Network::EventLoop* tLoopInThisThread = nullptr;

int createEventfd() {
  int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (event_fd < 0) {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return event_fd;
}

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};

IgnoreSigPipe initSignal;

}  // namespace

namespace rnet::Network {
EventLoop* EventLoop::getEventLoopOfCurrentThread() {
  return tLoopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(Thread::tid()),
      poller_(new Epoll(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
  if (tLoopInThisThread) {
    LOG_FATAL << "Another EventLoop " << tLoopInThisThread
              << " exists in this thread " << threadId_;
  } else {
    tLoopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeUp, this));
  // we are always reading the wakeup fd
  wakeupChannel_->enableReading();
}

}  // namespace rnet::Network