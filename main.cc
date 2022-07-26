#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>

#include <iostream>
#include <string>
#include <vector>

#include "detail/Time.h"
using namespace std;

int main() {
  auto t = rnet::detail::Timestamp::now();
  fmt::print("{}\n", t.toFormattedString());
  fmt::print("{}\n", t.secondsSinceEpoch());
  fmt::print("{}\n", t.toString());
  fmt::print("{}\n", t.toChronoSysTime().time_since_epoch().count());
}
