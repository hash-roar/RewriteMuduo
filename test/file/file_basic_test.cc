#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

#include "file/File.h"

using namespace rnet::File;

TEST(FILE_TEST, TEST_WRITE_READ) {
  std::string_view file_name = "test_file";
  ::remove(file_name.data());
  // test write a new file
  AppendFile file(file_name);
  char buf[] = "this is a message\n";
  for (int i = 999; i >= 0; i--) {
    file.append(buf, strlen(buf));
  }
  file.flush();

  std::fstream fs;
  fs.open(file_name.data());
  // should can be opened
  ASSERT_TRUE(fs.is_open());
  int line_count = 0;
  std::string content;
  content.resize(20);
  buf[strlen(buf) - 1] = 0;
  while (fs.getline(content.data(), content.size())) {
    EXPECT_STREQ(content.data(), buf);
    line_count++;
  }
  EXPECT_EQ(1000, line_count);

  // TEST READ
  FileReader reader(file_name);
  int read_num;
  reader.readToBuffer(&read_num);
  EXPECT_EQ(read_num, line_count * sizeof(buf));
}
