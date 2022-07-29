#include "file/File.h"

#include <fmt/core.h>

#include <cassert>
#include <string>
#include <utility>
using namespace rnet::File;

using Result = std::pair<std::string, bool>;

Result test_appendFile() {
  AppendFile file("file.txt");
  auto str = std::string(10, 'a');
  file.append(str.c_str(), str.size());
  assert(file.writtenBytes() == str.size());
  return {"append file test pass", true};
}

Result test_readFile() {
  auto buf = std::string{};
  buf.resize(10);
  fmt::print("{}\n", buf.size());
  auto str = readFile("file.txt", buf.size(), &buf);
  fmt::print("{}\n", buf);
  return {"read file success", true};
}

int main() {
  auto [res_str, success] = test_appendFile();
  test_readFile();
  fmt::print("{}", res_str);

  return 0;
}