/* game.cc — pure Minesweeper game logic (Stream B.1).
 *
 * Mirrors winmine rtns.c semantics. Orthodox C++: C-style, POD, pointers,
 * index loops, no STL, no exceptions, return codes. See game.h for contract.
 */
#include "minesweeper/game.h"

#include <string.h>

/* Deterministic fallback when no RNG is injected. A stateful LCG advanced on
 * every draw so successive calls differ — never loops, never crashes. The
 * state lives on the Board (b->rng_state), so the draw is reentrant and a given
 * board is reproducible from its state. */
static uint32_t game_fallback_rng(struct Board *b, uint32_t n) {
  b->rng_state = b->rng_state * 1664525u + 1013904223u;
  return (b->rng_state >> 8) % n;
}

static uint32_t game_rand(struct Board *b, uint32_t n) {
  if (n == 0) {
    return 0;
  }
  if (b->rng != NULL) {
    return b->rng(b->rng_ctx, n) % n;
  }
  return game_fallback_rng(b, n);
}

/* Per-process seed source, advanced once per game_reset so successive games get
 * a distinct fallback layout. WARNING: process-global — never write a test that
 * asserts an exact FALLBACK layout; it would be order-dependent under gtest
 * shuffle. Tests needing a fixed layout must inject `rng` (scripted). */
static uint32_t g_seed_source = 0x12345678u;

static int game_count_adjacent_mines(const struct Board *b, int x, int y) {
  int count = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      int nx = x + dx;
      int ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= b->width || ny >= b->height) {
        continue;
      }
      if (b->cells[game_index(b, nx, ny)].mine) {
        ++count;
      }
    }
  }
  return count;
}

void game_reset(struct Board *b, int w, int h, int mines, RngFn rng,
                void *rng_ctx) {
  memset(b, 0, sizeof *b);
  b->width = w;
  b->height = h;
  b->mines = mines;
  b->status = GAME_READY;
  b->revealed_count = 0;
  b->flag_count = 0;
  b->mines_placed = false;
  b->rng = rng;
  b->rng_ctx = rng_ctx;
  g_seed_source = g_seed_source * 1664525u + 1013904223u;
  b->rng_state = g_seed_source;
}

void game_place_mines(struct Board *b, int avoid_x, int avoid_y) {
  int total = b->width * b->height;
  int to_place = b->mines;
  /* Cannot place more mines than there are non-avoided cells. */
  if (to_place > total - 1) {
    to_place = total - 1;
  }

  int placed = 0;
  while (placed < to_place) {
    int x = (int)game_rand(b, (uint32_t)b->width);
    int y = (int)game_rand(b, (uint32_t)b->height);
    if (x == avoid_x && y == avoid_y) {
      continue;
    }
    int idx = game_index(b, x, y);
    if (b->cells[idx].mine) {
      continue;
    }
    b->cells[idx].mine = true;
    ++placed;
  }

  /* Compute adjacency for every non-mine cell. */
  for (int y = 0; y < b->height; ++y) {
    for (int x = 0; x < b->width; ++x) {
      int idx = game_index(b, x, y);
      if (b->cells[idx].mine) {
        b->cells[idx].adjacent = 0;
      } else {
        b->cells[idx].adjacent = (uint8_t)game_count_adjacent_mines(b, x, y);
      }
    }
  }

  b->mines_placed = true;
}

static bool game_in_range(const struct Board *b, int x, int y) {
  return x >= 0 && y >= 0 && x < b->width && y < b->height;
}

/* Reveal a single covered, non-flagged cell. Returns the count of newly
 * revealed cells (0 if it was a no-op). Mines are revealed too (caller checks
 * for loss). Fills queue with newly revealed zero-adjacent cells for flood. */
static int game_step_one(struct Board *b, int x, int y) {
  if (!game_in_range(b, x, y)) {
    return 0;
  }
  int idx = game_index(b, x, y);
  struct Cell *c = &b->cells[idx];
  if (c->revealed || c->flag == FLAG_MINE) {
    return 0;
  }
  c->revealed = true;
  if (!c->mine) {
    ++b->revealed_count;
  }
  return 1;
}

/* Iterative flood fill (StepBox analogue). Starts from (x,y); reveals connected
 * zero-adjacent regions and their numbered borders. No recursion. */
static void game_flood(struct Board *b, int x, int y) {
  int qx[BOARD_MAX_CELLS];
  int qy[BOARD_MAX_CELLS];
  int head = 0;
  int tail = 0;

  if (game_step_one(b, x, y) == 0) {
    return;
  }
  if (b->cells[game_index(b, x, y)].adjacent == 0 &&
      !b->cells[game_index(b, x, y)].mine) {
    qx[tail] = x;
    qy[tail] = y;
    ++tail;
  }

  while (head < tail) {
    int cx = qx[head];
    int cy = qy[head];
    ++head;
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) {
          continue;
        }
        int nx = cx + dx;
        int ny = cy + dy;
        if (game_step_one(b, nx, ny) == 0) {
          continue;
        }
        int nidx = game_index(b, nx, ny);
        if (b->cells[nidx].adjacent == 0 && !b->cells[nidx].mine) {
          qx[tail] = nx;
          qy[tail] = ny;
          ++tail;
        }
      }
    }
  }
}

static bool game_all_clear(const struct Board *b) {
  return b->revealed_count == (b->width * b->height) - b->mines;
}

/* On win: auto-flag remaining mine cells so the counter reads zero (original
 * UpdateBombCount(-cBombLeft) behavior). Keeps flag_count consistent. */
static void game_finish_win(struct Board *b) {
  b->status = GAME_WON;
  for (int i = 0; i < b->width * b->height; ++i) {
    if (b->cells[i].mine && b->cells[i].flag != FLAG_MINE) {
      b->cells[i].flag = FLAG_MINE;
      ++b->flag_count;
    }
  }
}

int game_reveal(struct Board *b, int x, int y) {
  if (b->status == GAME_WON || b->status == GAME_LOST) {
    return REVEAL_NONE;
  }
  if (!game_in_range(b, x, y)) {
    return REVEAL_NONE;
  }

  if (b->status == GAME_READY) {
    if (!b->mines_placed) {
      game_place_mines(b, x, y);
    }
    b->status = GAME_PLAYING;
  }

  int idx = game_index(b, x, y);
  struct Cell *c = &b->cells[idx];
  if (c->revealed || c->flag == FLAG_MINE) {
    return REVEAL_NONE;
  }

  if (c->mine) {
    c->revealed = true;
    c->exploded = true;
    b->status = GAME_LOST;
    return REVEAL_LOSS;
  }

  game_flood(b, x, y);

  if (game_all_clear(b)) {
    game_finish_win(b);
    return REVEAL_WIN;
  }
  return REVEAL_OK;
}

static int game_count_flag_mine(const struct Board *b, int x, int y) {
  int count = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      int nx = x + dx;
      int ny = y + dy;
      if (!game_in_range(b, nx, ny)) {
        continue;
      }
      if (b->cells[game_index(b, nx, ny)].flag == FLAG_MINE) {
        ++count;
      }
    }
  }
  return count;
}

int game_chord(struct Board *b, int x, int y) {
  if (b->status != GAME_PLAYING) {
    return REVEAL_NONE;
  }
  if (!game_in_range(b, x, y)) {
    return REVEAL_NONE;
  }
  int idx = game_index(b, x, y);
  struct Cell *c = &b->cells[idx];
  if (!c->revealed || c->mine) {
    return REVEAL_NONE;
  }
  if ((int)game_count_flag_mine(b, x, y) != (int)c->adjacent) {
    return REVEAL_NONE;
  }

  bool hit_mine = false;
  bool revealed_any = false;

  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      int nx = x + dx;
      int ny = y + dy;
      if (!game_in_range(b, nx, ny)) {
        continue;
      }
      int nidx = game_index(b, nx, ny);
      struct Cell *nc = &b->cells[nidx];
      if (nc->flag == FLAG_MINE || nc->revealed) {
        continue;
      }
      if (nc->mine) {
        /* wrong flag elsewhere -> detonate this mine */
        nc->revealed = true;
        nc->exploded = true;
        hit_mine = true;
      } else {
        game_flood(b, nx, ny);
        revealed_any = true;
      }
    }
  }

  if (hit_mine) {
    b->status = GAME_LOST;
    return REVEAL_LOSS;
  }
  if (game_all_clear(b)) {
    game_finish_win(b);
    return REVEAL_WIN;
  }
  if (revealed_any) {
    return REVEAL_OK;
  }
  return REVEAL_NONE;
}

void game_cycle_flag(struct Board *b, int x, int y, bool marks_enabled) {
  if (b->status != GAME_READY && b->status != GAME_PLAYING) {
    return;
  }
  if (!game_in_range(b, x, y)) {
    return;
  }
  int idx = game_index(b, x, y);
  struct Cell *c = &b->cells[idx];
  if (c->revealed) {
    return;
  }

  if (c->flag == FLAG_NONE) {
    c->flag = FLAG_MINE;
    ++b->flag_count;
  } else if (c->flag == FLAG_MINE) {
    --b->flag_count;
    if (marks_enabled) {
      c->flag = FLAG_QUESTION;
    } else {
      c->flag = FLAG_NONE;
    }
  } else { /* FLAG_QUESTION */
    c->flag = FLAG_NONE;
  }
}

int game_mines_remaining(const struct Board *b) {
  return b->mines - b->flag_count;
}
