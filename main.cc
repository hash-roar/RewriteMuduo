#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>

#include <iostream>
#include <string>
#include <vector>

#include "Logger.h"
#include "detail/Thread.h"
#include "detail/Time.h"
using namespace std;

using namespace rnet;

int main() {
  rnet::Thread::Thread t([] {
    fmt::print("this is from another thread\n");
    fmt::print("thread name:{}\n", Thread::tThreadName);
    fmt::print("thread id:{}\n", Thread::tid());
  });
  t.start();

  fmt::print("main thread get thread id:{}\n", t.gettid());
  fmt::print("create thread num:{}\n", Thread::Thread::numCreated());
  fmt::print("main thread\n");
  fmt::print("main thread name:{}\n", Thread::tThreadName);
  fmt::print("main thread id:{}\n", Thread::tid());

  t.join();
  return 0;
}
