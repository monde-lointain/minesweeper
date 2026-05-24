/* config_test.cc — STUB (Stream A skeleton). Replaced by Stream B.2 with the
 * headless save->load round-trip. */
#include <gtest/gtest.h>

#include "minesweeper/config.h"

TEST(ConfigTest, DefaultsAreSane) {
  Settings s;
  config_defaults(&s);
  EXPECT_EQ(s.difficulty, DIFF_BEGINNER);
  EXPECT_EQ(s.scale, 2);
  EXPECT_TRUE(s.color);
}
