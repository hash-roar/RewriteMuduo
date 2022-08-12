#include <cassert>
#include <iostream>

#include "unix/Duration.h"
using namespace rnet::Unix;
int main() {
  Duration dur{1003254213};
  std::cout << dur.Nanoseconds() << "\n";
  std::cout << dur.Microseconds() << "\n";
  std::cout << dur.Milliseconds() << "\n";
  std::cout << dur.Seconds() << "\n";
  std::cout << dur.TimeVal().tv_sec << "\n";
  assert(dur == Duration{1003254213});
  return 0;
}