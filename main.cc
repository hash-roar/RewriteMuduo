#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>

#include <iostream>
#include <string>
#include <vector>

#include "Logger.h"
#include "detail/Time.h"
using namespace std;

using namespace rnet;

int main() {
  // LOG_INFO << fmt::format("this is log {}", "fgeada");
  rnet::Logger("filename/fasfda", 11).stream()
      << fmt::format("this is log {}sacccccccccccccccccccccc", "fgeada");
  return 0;
}
