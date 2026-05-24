/* game_test.cc — pure game-logic unit tests (Stream B.1).
 * GoogleTest; NOT plugin-checked, so normal C++ is fine here.
 */
#include "minesweeper/game.h"

#include <gtest/gtest.h>

#include <vector>

/* ---- Scripted RNG ------------------------------------------------------- */
/* Returns scripted values in sequence; each value is reduced mod n by the
 * caller (game_rand applies % n). When exhausted, returns 0. */
struct ScriptRng {
  std::vector<uint32_t> vals;
  size_t i = 0;
};

static uint32_t script_rng(void *ctx, uint32_t n) {
  ScriptRng *s = static_cast<ScriptRng *>(ctx);
  if (n == 0) return 0;
  if (s->i >= s->vals.size()) return 0;
  return s->vals[s->i++];
}

/* Place mines at an explicit list of (x,y) coords by scripting the rng:
 * game_place_mines draws x then y per attempt and skips dups/avoided. */
static void script_coords(ScriptRng *s,
                          std::initializer_list<std::pair<int, int>> coords) {
  for (auto &c : coords) {
    s->vals.push_back(static_cast<uint32_t>(c.first));
    s->vals.push_back(static_cast<uint32_t>(c.second));
  }
}

static int mine_count(const Board &b) {
  int n = 0;
  for (int i = 0; i < b.width * b.height; ++i)
    if (b.cells[i].mine) ++n;
  return n;
}

/* ---- Reset -------------------------------------------------------------- */

TEST(Reset, InitializesGeometryAndState) {
  Board b;
  game_reset(&b, 16, 12, 40, nullptr, nullptr);
  EXPECT_EQ(b.width, 16);
  EXPECT_EQ(b.height, 12);
  EXPECT_EQ(b.mines, 40);
  EXPECT_EQ(b.status, GAME_READY);
  EXPECT_EQ(b.revealed_count, 0);
  EXPECT_EQ(b.flag_count, 0);
  EXPECT_FALSE(b.mines_placed);
}

TEST(Reset, ClearsAllCells) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  for (int i = 0; i < 9 * 9; ++i) {
    EXPECT_FALSE(b.cells[i].mine);
    EXPECT_FALSE(b.cells[i].revealed);
    EXPECT_FALSE(b.cells[i].exploded);
    EXPECT_EQ(b.cells[i].flag, FLAG_NONE);
    EXPECT_EQ(b.cells[i].adjacent, 0);
  }
}

/* ---- Mine placement ----------------------------------------------------- */

TEST(Placement, PlacesExactMineCount) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  game_place_mines(&b, 0, 0);
  EXPECT_EQ(mine_count(b), 10);
  EXPECT_TRUE(b.mines_placed);
}

TEST(Placement, NeverOnAvoidedCell) {
  /* Try every cell as the avoid target across many fallback placements. */
  for (int trial = 0; trial < 50; ++trial) {
    Board b;
    game_reset(&b, 9, 9, 10, nullptr, nullptr);
    int ax = trial % 9;
    int ay = (trial / 9) % 9;
    game_place_mines(&b, ax, ay);
    EXPECT_FALSE(b.cells[game_index(&b, ax, ay)].mine)
        << "avoided (" << ax << "," << ay << ")";
    EXPECT_EQ(mine_count(b), 10);
  }
}

TEST(Placement, ScriptedRngHonoredAndSkipsAvoided) {
  Board b;
  ScriptRng s;
  /* Want mines at (1,1),(2,2),(3,3). Script avoided (0,0) first -> skipped. */
  script_coords(&s, {{0, 0}, {1, 1}, {2, 2}, {3, 3}});
  game_reset(&b, 9, 9, 3, script_rng, &s);
  game_place_mines(&b, 0, 0);
  EXPECT_FALSE(b.cells[game_index(&b, 0, 0)].mine);
  EXPECT_TRUE(b.cells[game_index(&b, 1, 1)].mine);
  EXPECT_TRUE(b.cells[game_index(&b, 2, 2)].mine);
  EXPECT_TRUE(b.cells[game_index(&b, 3, 3)].mine);
  EXPECT_EQ(mine_count(b), 3);
}

TEST(Placement, AdjacencyCountsCorrect) {
  Board b;
  ScriptRng s;
  /* Mines forming an L around center (1,1) on a 3x3:
   * mines at (0,0),(1,0),(0,1). Center (1,1) should see 3 adjacent. */
  script_coords(&s, {{0, 0}, {1, 0}, {0, 1}});
  game_reset(&b, 3, 3, 3, script_rng, &s);
  game_place_mines(&b, 2, 2);
  EXPECT_EQ(b.cells[game_index(&b, 1, 1)].adjacent, 3);
  EXPECT_EQ(b.cells[game_index(&b, 2, 0)].adjacent, 1);
  EXPECT_EQ(b.cells[game_index(&b, 2, 1)].adjacent, 1);
  EXPECT_EQ(b.cells[game_index(&b, 2, 2)].adjacent, 0);
  EXPECT_EQ(b.cells[game_index(&b, 1, 2)].adjacent, 1);
}

/* ---- Flood fill --------------------------------------------------------- */

TEST(Flood, ZeroRegionRevealsConnectedAreaAndBorder) {
  /* 5x5, single mine at corner (4,4). Click far corner (0,0): the whole board
   * except the mine should reveal (flood + numbered border). */
  Board b;
  ScriptRng s;
  script_coords(&s, {{4, 4}});
  game_reset(&b, 5, 5, 1, script_rng, &s);
  int r = game_reveal(&b, 0, 0);
  /* All 24 non-mine cells revealed -> win. */
  EXPECT_EQ(r, REVEAL_WIN);
  EXPECT_EQ(b.revealed_count, 24);
  EXPECT_FALSE(b.cells[game_index(&b, 4, 4)].revealed);
}

TEST(Flood, BorderNumbersStopExpansion) {
  /* 5x5 mine at (2,2). Click (0,0): flood reveals zeros up to the ring of
   * numbered cells around the mine, but not the mine. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{2, 2}});
  game_reset(&b, 5, 5, 1, script_rng, &s);
  int r = game_reveal(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_WIN);
  EXPECT_EQ(b.revealed_count, 24);
  /* The 8 cells around the mine are revealed numbers. */
  EXPECT_TRUE(b.cells[game_index(&b, 1, 1)].revealed);
  EXPECT_EQ(b.cells[game_index(&b, 1, 1)].adjacent, 1);
}

TEST(Flood, NonZeroClickRevealsOnlyOne) {
  /* Mine adjacent to clicked cell -> clicked is a number -> only it reveals. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  int r = game_reveal(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_OK);
  EXPECT_EQ(b.revealed_count, 1);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].adjacent, 1);
}

/* ---- Reveal results / loss --------------------------------------------- */

TEST(Reveal, FirstClickPlacesMinesAndIsSafe) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  int r = game_reveal(&b, 4, 4);
  EXPECT_NE(r, REVEAL_LOSS);
  EXPECT_TRUE(b.mines_placed);
  EXPECT_EQ(b.status == GAME_PLAYING || b.status == GAME_WON, true);
  EXPECT_FALSE(b.cells[game_index(&b, 4, 4)].mine);
}

TEST(Reveal, HittingMineLoses) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8); /* first click safe, places mine at (1,0) */
  int r = game_reveal(&b, 1, 0);
  EXPECT_EQ(r, REVEAL_LOSS);
  EXPECT_EQ(b.status, GAME_LOST);
  EXPECT_TRUE(b.cells[game_index(&b, 1, 0)].exploded);
  EXPECT_TRUE(b.cells[game_index(&b, 1, 0)].revealed);
}

TEST(Reveal, FlaggedCellIsNoOp) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8);
  game_cycle_flag(&b, 0, 0, false);
  int r = game_reveal(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_NONE);
  EXPECT_FALSE(b.cells[game_index(&b, 0, 0)].revealed);
}

TEST(Reveal, AlreadyRevealedIsNoOp) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8);
  int r = game_reveal(&b, 8, 8);
  EXPECT_EQ(r, REVEAL_NONE);
}

TEST(Reveal, OutOfRangeIsNoOp) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  EXPECT_EQ(game_reveal(&b, -1, 0), REVEAL_NONE);
  EXPECT_EQ(game_reveal(&b, 9, 0), REVEAL_NONE);
  EXPECT_EQ(game_reveal(&b, 0, 9), REVEAL_NONE);
}

TEST(Reveal, AfterGameOverIsNoOp) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8);
  game_reveal(&b, 1, 0); /* lose */
  int r = game_reveal(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_NONE);
}

/* ---- Win ---------------------------------------------------------------- */

TEST(Win, RevealAllNonMinesWinsAndAutoFlags) {
  /* 3x3, mine at (2,2). Reveal everything else -> win, auto-flag the mine. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{2, 2}});
  game_reset(&b, 3, 3, 1, script_rng, &s);
  int r = game_reveal(&b, 0, 0); /* (0,0) is a 0 -> floods most */
  EXPECT_EQ(r, REVEAL_WIN);
  EXPECT_EQ(b.status, GAME_WON);
  EXPECT_EQ(b.cells[game_index(&b, 2, 2)].flag, FLAG_MINE);
  EXPECT_EQ(b.flag_count, 1);
  EXPECT_EQ(game_mines_remaining(&b), 0);
}

/* ---- Flag cycle --------------------------------------------------------- */

TEST(Flag, CycleWithMarks) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  game_cycle_flag(&b, 0, 0, true);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].flag, FLAG_MINE);
  EXPECT_EQ(b.flag_count, 1);
  game_cycle_flag(&b, 0, 0, true);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].flag, FLAG_QUESTION);
  EXPECT_EQ(b.flag_count, 0);
  game_cycle_flag(&b, 0, 0, true);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].flag, FLAG_NONE);
  EXPECT_EQ(b.flag_count, 0);
}

TEST(Flag, CycleWithoutMarks) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  game_cycle_flag(&b, 0, 0, false);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].flag, FLAG_MINE);
  EXPECT_EQ(b.flag_count, 1);
  game_cycle_flag(&b, 0, 0, false);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].flag, FLAG_NONE);
  EXPECT_EQ(b.flag_count, 0);
}

TEST(Flag, RevealedCellNotFlaggable) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8);
  /* (8,8) is now revealed (part of flood); flagging is a no-op. */
  int before = b.flag_count;
  game_cycle_flag(&b, 8, 8, true);
  EXPECT_EQ(b.flag_count, before);
  EXPECT_EQ(b.cells[game_index(&b, 8, 8)].flag, FLAG_NONE);
}

TEST(Flag, MinesRemainingGoesNegativeWhenOverFlagged) {
  Board b;
  game_reset(&b, 9, 9, 2, nullptr, nullptr);
  game_cycle_flag(&b, 0, 0, false);
  game_cycle_flag(&b, 1, 0, false);
  game_cycle_flag(&b, 2, 0, false);
  EXPECT_EQ(b.flag_count, 3);
  EXPECT_EQ(game_mines_remaining(&b), -1);
}

/* ---- Chord -------------------------------------------------------------- */

TEST(Chord, RevealsWhenFlagCountMatches) {
  /* 9x9, single mine at (1,0). Reveal (0,0) -> number 1. Flag the mine.
   * Chord on (0,0) reveals remaining neighbours safely. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 0, 0);
  EXPECT_EQ(b.cells[game_index(&b, 0, 0)].adjacent, 1);
  game_cycle_flag(&b, 1, 0, false); /* correct flag on the mine */
  int r = game_chord(&b, 0, 0);
  /* Neighbours (0,1),(1,1) revealed; they're zeros -> flood -> likely win. */
  EXPECT_TRUE(r == REVEAL_OK || r == REVEAL_WIN);
  EXPECT_TRUE(b.cells[game_index(&b, 0, 1)].revealed);
}

TEST(Chord, NoOpWhenFlagCountMismatch) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 0, 0); /* number 1, no flags placed */
  int r = game_chord(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_NONE);
  EXPECT_FALSE(b.cells[game_index(&b, 0, 1)].revealed);
}

TEST(Chord, WrongFlagDetonates) {
  /* Mine at (1,0). Flag the WRONG cell (0,1). Reveal (0,0)=1. Chord matches
   * the count (1 flag) but the real mine is unflagged -> detonate. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 0, 0);
  game_cycle_flag(&b, 0, 1, false); /* wrong flag, but count==1==number */
  int r = game_chord(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_LOSS);
  EXPECT_EQ(b.status, GAME_LOST);
  EXPECT_TRUE(b.cells[game_index(&b, 1, 0)].exploded);
}

TEST(Chord, OnUnrevealedIsNoOp) {
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 8, 8);
  int r = game_chord(&b, 0, 0); /* (0,0) not revealed */
  EXPECT_EQ(r, REVEAL_NONE);
}

TEST(Chord, QuestionMarkDoesNotCount) {
  /* Mine at (1,0). Reveal (0,0)=1. Put a QUESTION on the mine cell -> it must
   * NOT count toward the flag total, so chord is a no-op. */
  Board b;
  ScriptRng s;
  script_coords(&s, {{1, 0}});
  game_reset(&b, 9, 9, 1, script_rng, &s);
  game_reveal(&b, 0, 0);
  game_cycle_flag(&b, 1, 0, true); /* -> FLAG_MINE */
  game_cycle_flag(&b, 1, 0, true); /* -> FLAG_QUESTION (count back to 0) */
  EXPECT_EQ(b.cells[game_index(&b, 1, 0)].flag, FLAG_QUESTION);
  int r = game_chord(&b, 0, 0);
  EXPECT_EQ(r, REVEAL_NONE);
}

/* ---- mines_remaining ---------------------------------------------------- */

TEST(MinesRemaining, EqualsMinesMinusFlagCount) {
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  EXPECT_EQ(game_mines_remaining(&b), 10);
  game_cycle_flag(&b, 0, 0, false);
  EXPECT_EQ(game_mines_remaining(&b), 9);
}
