#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/ranges.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
  std::vector vec{1, 2};
  fmt::print(
"{}", vec);
  return 0;
}
