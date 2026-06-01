/* types.h — common POD types, enums, and limits shared across modules.
 *
 * Orthodox C++: plain structs, plain enums (UPPER_SNAKE members), C headers,
 * no namespace. This header is a FROZEN CONTRACT (Stream A); fan-out streams
 * implement against it and must not edit it.
 */
#ifndef MINESWEEPER_TYPES_H
#define MINESWEEPER_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/* Difficulty presets. Custom uses Settings-provided dimensions. */
enum Difficulty {
  DIFF_BEGINNER = 0,
  DIFF_INTERMEDIATE = 1,
  DIFF_EXPERT = 2,
  DIFF_CUSTOM = 3
};

/* Board dimension / mine limits (minima mirror original winmine; maxima
 * raised well beyond it). */
enum {
  BOARD_MIN_W = 9,
  BOARD_MIN_H = 9,
  BOARD_MAX_W = 100,
  BOARD_MAX_H = 100,
  BOARD_MIN_MINES = 10,
  BOARD_MAX_MINES = 2500, /* global ceiling; per-board cap is min(this, w*h-1) */
  BOARD_MAX_CELLS = BOARD_MAX_W * BOARD_MAX_H,
  SCORE_NAME_MAX = 32 /* per high-score name, incl. NUL */
};

/* Beginner/Intermediate/Expert preset table indices. */
enum {
  LEVEL_COUNT = 3 /* Custom does not record best times */
};

/* Settings: the full persisted state. POD only — the public face of `config`.
 * mINI/STL never appears here.
 */
struct Settings {
  int difficulty;    /* enum Difficulty */
  int custom_w;      /* clamped BOARD_MIN_W..BOARD_MAX_W */
  int custom_h;      /* clamped BOARD_MIN_H..BOARD_MAX_H */
  int custom_mines;  /* clamped BOARD_MIN_MINES..(w*h - 1) */
  bool sound;        /* play win/explode */
  bool marks;        /* allow the (?) flag state */
  bool color;        /* true=color sprites, false=B&W */
  int scale;         /* integer render scale, 1..4 */
  bool menu_visible; /* ImGui main menu bar shown */
  int window_x;      /* last window position */
  int window_y;
  int best_time[LEVEL_COUNT]; /* seconds, 999 = none */
  char best_name[LEVEL_COUNT][SCORE_NAME_MAX];
};

#endif /* MINESWEEPER_TYPES_H */
