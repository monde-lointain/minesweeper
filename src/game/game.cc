/* game.cc — STUB (Stream A skeleton). Replaced by Stream B.1 with the full
 * winmine logic. Trivial Orthodox-clean implementations satisfying game.h. */
#include "minesweeper/game.h"

#include <string.h>

void game_reset(struct Board *b, int w, int h, int mines, RngFn rng, void *rng_ctx) {
    memset(b, 0, sizeof *b);
    b->width = w;
    b->height = h;
    b->mines = mines;
    b->status = GAME_READY;
    b->rng = rng;
    b->rng_ctx = rng_ctx;
}

void game_place_mines(struct Board *b, int avoid_x, int avoid_y) {
    (void)b;
    (void)avoid_x;
    (void)avoid_y;
}

int game_reveal(struct Board *b, int x, int y) {
    (void)b;
    (void)x;
    (void)y;
    return REVEAL_NONE;
}

int game_chord(struct Board *b, int x, int y) {
    (void)b;
    (void)x;
    (void)y;
    return REVEAL_NONE;
}

void game_cycle_flag(struct Board *b, int x, int y, bool marks_enabled) {
    (void)b;
    (void)x;
    (void)y;
    (void)marks_enabled;
}

int game_mines_remaining(const struct Board *b) {
    return b->mines - b->flag_count;
}
