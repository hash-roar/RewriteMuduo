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
  rnet::Thread::Thread t([] { fmt::print("this is from another thread\n"); });

  t.start();
  t.join();
  return 0;
}
