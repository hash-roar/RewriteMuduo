#include <gtest/gtest.h>

TEST(test_thread, thread_init) {
  EXPECT_EQ(1, 1);
  EXPECT_EQ(2, 1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}