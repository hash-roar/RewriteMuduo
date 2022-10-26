#include "unix/Time.h"

#include <gtest/gtest.h>
using namespace rnet::Unix;

TEST(TIME_TEST, TEST_ALL_API) {
  Timestamp now = Timestamp::now();
  // EXPECT_EQ(now, val2)
  ASSERT_TRUE(now.valid());

  // test swap
  auto that = now;
  ASSERT_EQ(that, now);
  that.swap(that);
  ASSERT_EQ(that, now);
  ASSERT_EQ(0, timeDifference(that, now));

  // test chrono
  auto chrono = now.toChronoSysTime();
  ASSERT_EQ(chrono.time_since_epoch().count() / 1000,
            now.microSecondsSinceEpoch());
}