#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
std::vector<std::string_view> splitStringView(std::string_view str_view,
                                              char delimeter) {
  // std::cout << str_view << "\n";
  std::vector<std::string_view> result{};
  if (str_view.empty()) {
    return result;
  }
  auto pos = str_view.find(delimeter);
  size_t last_pos = 0;
  while (pos != std::string::npos) {
    result.emplace_back(str_view.begin() + last_pos, pos - last_pos);
    last_pos = pos + 1;
    pos = str_view.find(delimeter, last_pos);
  }
  result.emplace_back(str_view.begin() + last_pos, str_view.size() - last_pos);
  return result;
}

int main() {
  auto str_vec = splitStringView("    ", ' ');
  for (const auto &str : str_vec) {
    std::cout << str.size() << str << "\n";
  }
  return -1;
}