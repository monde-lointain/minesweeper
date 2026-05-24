/* game_test.cc — STUB (Stream A skeleton). Replaced by Stream B.1 with full
 * game-logic coverage. */
#include <gtest/gtest.h>

#include "minesweeper/game.h"

TEST(GameTest, ResetInitializesGeometry) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  EXPECT_EQ(b.width, 9);
  EXPECT_EQ(b.height, 9);
  EXPECT_EQ(b.mines, 10);
  EXPECT_EQ(b.status, GAME_READY);
}
