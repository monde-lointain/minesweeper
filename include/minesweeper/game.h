/* game.h — pure game logic contract. NO SDL/ImGui. Unit-tested.
 *
 * Mirrors winmine rtns.c semantics. FROZEN CONTRACT (Stream A).
 * See ~/development/projects/c/winmine/rtns.c for reference behavior.
 *
 * Post-Stream-A amendment (authorized; see docs/2026-05-23-minesweeper-port-
 * design.md): added Board.rng_state so the fallback RNG is board-local.
 */
#ifndef MINESWEEPER_GAME_H
#define MINESWEEPER_GAME_H

#include "minesweeper/types.h"

/* Per-cell flag state (right-click cycle: none -> mine -> question -> none). */
enum CellFlag { FLAG_NONE = 0, FLAG_MINE = 1, FLAG_QUESTION = 2 };

/* Overall game state. READY = no reveal yet (timer stopped, mines unplaced). */
enum GameStatus {
  GAME_READY = 0,
  GAME_PLAYING = 1,
  GAME_WON = 2,
  GAME_LOST = 3
};

/* Result of a reveal/chord action. */
enum RevealResult {
  REVEAL_NONE = 0, /* no-op (out of range / already revealed / flagged) */
  REVEAL_OK = 1,   /* cells revealed, game continues */
  REVEAL_WIN = 2,  /* this action won the game */
  REVEAL_LOSS = 3  /* this action hit a mine */
};

/* RNG injected for determinism. Returns a value in [0, n). */
typedef uint32_t (*RngFn)(void *ctx, uint32_t n);

struct Cell {
  uint8_t adjacent; /* count of adjacent mines, 0..8 (valid once revealed) */
  bool mine;
  bool revealed;
  bool exploded; /* the mine that was clicked (loss render) */
  uint8_t flag;  /* enum CellFlag */
};

struct Board {
  int width;
  int height;
  int mines;
  int status;         /* enum GameStatus */
  int revealed_count; /* non-mine cells revealed */
  int flag_count;     /* cells in FLAG_MINE state */
  bool mines_placed;
  struct Cell cells[BOARD_MAX_CELLS];
  RngFn rng;
  void *rng_ctx;
  uint32_t rng_state; /* fallback LCG state; board-local, reentrant */
};

/* Index helper: row-major, width-strided. Callers stay in [0,width)x[0,height).
 */
static inline int game_index(const struct Board *b, int x, int y) {
  return (y * b->width) + x;
}

/* Reset to a fresh, unplayed board of the given geometry. Mines are NOT placed
 * until the first reveal (first-click safety). rng/ctx drive placement. */
void game_reset(struct Board *b, int w, int h, int mines, RngFn rng,
                void *rng_ctx);

/* Place `mines` mines uniformly, never on (avoid_x,avoid_y); compute adjacency.
 * Normally called internally on first reveal; exposed for tests. */
void game_place_mines(struct Board *b, int avoid_x, int avoid_y);

/* Left-click reveal with flood-fill of zero-adjacent regions. On first reveal,
 * places mines avoiding (x,y). Returns enum RevealResult. */
int game_reveal(struct Board *b, int x, int y);

/* Both-button chord on a revealed number: if adjacent FLAG_MINE count ==
 * number, reveal all non-flagged neighbours. Returns enum RevealResult. */
int game_chord(struct Board *b, int x, int y);

/* Right-click cycle. With marks: none->mine->question->none; without:
 * none->mine->none. */
void game_cycle_flag(struct Board *b, int x, int y, bool marks_enabled);

/* mines - flag_count (may be negative). */
int game_mines_remaining(const struct Board *b);

#endif /* MINESWEEPER_GAME_H */
