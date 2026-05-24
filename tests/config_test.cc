/* config_test.cc — headless save->load round-trip + edge cases (GoogleTest). */
#include "minesweeper/config.h"

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

namespace {

const char *kPath = "/tmp/ms_cfg_test.ini";

void fill_nondefault(Settings *s) {
  memset(s, 0, sizeof *s);
  s->difficulty = DIFF_CUSTOM;
  s->custom_w = 24;
  s->custom_h = 20;
  s->custom_mines = 77;
  s->sound = false;
  s->marks = false;
  s->color = false;
  s->scale = 3;
  s->menu_visible = false;
  s->window_x = 123;
  s->window_y = 456;
  s->best_time[0] = 11;
  strcpy(s->best_name[0], "Ada Lovelace");
  s->best_time[1] = 222;
  strcpy(s->best_name[1], "Grace Hopper");
  s->best_time[2] = 333;
  strcpy(s->best_name[2], "Alan M Turing");
}

void expect_eq(const Settings *a, const Settings *b) {
  EXPECT_EQ(a->difficulty, b->difficulty);
  EXPECT_EQ(a->custom_w, b->custom_w);
  EXPECT_EQ(a->custom_h, b->custom_h);
  EXPECT_EQ(a->custom_mines, b->custom_mines);
  EXPECT_EQ(a->sound, b->sound);
  EXPECT_EQ(a->marks, b->marks);
  EXPECT_EQ(a->color, b->color);
  EXPECT_EQ(a->scale, b->scale);
  EXPECT_EQ(a->menu_visible, b->menu_visible);
  EXPECT_EQ(a->window_x, b->window_x);
  EXPECT_EQ(a->window_y, b->window_y);
  for (int i = 0; i < LEVEL_COUNT; ++i) {
    EXPECT_EQ(a->best_time[i], b->best_time[i]);
    EXPECT_STREQ(a->best_name[i], b->best_name[i]);
  }
}

}  // namespace

TEST(ConfigTest, DefaultsAreSane) {
  Settings s;
  config_defaults(&s);
  EXPECT_EQ(s.difficulty, DIFF_BEGINNER);
  EXPECT_EQ(s.custom_w, BOARD_MIN_W);
  EXPECT_EQ(s.custom_h, BOARD_MIN_H);
  EXPECT_EQ(s.custom_mines, BOARD_MIN_MINES);
  EXPECT_TRUE(s.sound);
  EXPECT_TRUE(s.marks);
  EXPECT_TRUE(s.color);
  EXPECT_EQ(s.scale, 2);
  EXPECT_TRUE(s.menu_visible);
  EXPECT_EQ(s.window_x, -1);
  EXPECT_EQ(s.window_y, -1);
  for (int i = 0; i < LEVEL_COUNT; ++i) {
    EXPECT_EQ(s.best_time[i], 999);
    EXPECT_STREQ(s.best_name[i], "Anonymous");
  }
}

TEST(ConfigTest, RoundTrip) {
  Settings out;
  fill_nondefault(&out);
  ASSERT_EQ(config_save(&out, kPath), 0);

  Settings in;
  ASSERT_EQ(config_load(&in, kPath), 0);
  expect_eq(&out, &in);
}

TEST(ConfigTest, LoadMissingYieldsDefaults) {
  Settings s;
  int rc = config_load(&s, "/tmp/ms_cfg_does_not_exist_42.ini");
  EXPECT_NE(rc, 0);

  Settings def;
  config_defaults(&def);
  expect_eq(&def, &s);
}

TEST(ConfigTest, ClampsOutOfRange) {
  /* Hand-write an INI with out-of-range scale and mines and a bad difficulty.
   */
  FILE *f = fopen(kPath, "w");
  ASSERT_NE(f, nullptr);
  fprintf(f,
          "[game]\n"
          "difficulty=99\n"
          "custom_w=9\n"
          "custom_h=9\n"
          "custom_mines=5000\n"
          "scale=9\n");
  fclose(f);

  Settings s;
  ASSERT_EQ(config_load(&s, kPath), 0);
  EXPECT_EQ(s.scale, 4);                /* clamped 1..4 */
  EXPECT_EQ(s.difficulty, DIFF_CUSTOM); /* clamped 0..3 */
  EXPECT_EQ(s.custom_mines, 9 * 9 - 1); /* clamped to w*h-1 */
}

TEST(ConfigTest, MissingKeysKeepDefaults) {
  FILE *f = fopen(kPath, "w");
  ASSERT_NE(f, nullptr);
  fprintf(f, "[game]\nscale=4\n");
  fclose(f);

  Settings s;
  ASSERT_EQ(config_load(&s, kPath), 0);
  EXPECT_EQ(s.scale, 4);
  /* untouched keys retain defaults */
  EXPECT_EQ(s.difficulty, DIFF_BEGINNER);
  EXPECT_TRUE(s.sound);
  EXPECT_EQ(s.best_time[0], 999);
  EXPECT_STREQ(s.best_name[0], "Anonymous");
}
