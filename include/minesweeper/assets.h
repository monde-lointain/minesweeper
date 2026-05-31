/* assets.h — sprite-sheet textures + sub-rect tables. FROZEN CONTRACT (Stream
 * A).
 *
 * Original sheets (single vertical column each), see ~/.../winmine/bmp/:
 *   blocks.bmp 16x256  -> 16 tiles  @16x16  (enum CellSprite, rtns.h iBlk*)
 *   led.bmp    13x276  -> 12 digits @13x23  (0-9, blank=10, negative=11)
 *   button.bmp 24x120  ->  5 faces  @24x24  (enum ButtonSprite)
 * Color + B&W variants both loaded; active set chosen by Settings.color.
 */
#ifndef MINESWEEPER_ASSETS_H
#define MINESWEEPER_ASSETS_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/* Block sprite indices (match winmine rtns.h). */
enum CellSprite {
  SPR_BLANK = 0, /* revealed, 0 adjacent */
  SPR_1 = 1,
  SPR_2 = 2,
  SPR_3 = 3,
  SPR_4 = 4,
  SPR_5 = 5,
  SPR_6 = 6,
  SPR_7 = 7,
  SPR_8 = 8,
  SPR_GUESS_DN = 9,  /* ? pressed */
  SPR_BOMB_DN = 10,  /* flag pressed */
  SPR_WRONG = 11,    /* wrongly-flagged mine (loss) */
  SPR_EXPLODE = 12,  /* detonated mine (loss) */
  SPR_GUESS_UP = 13, /* ? */
  SPR_BOMB_UP = 14,  /* flag */
  SPR_BLANK_UP = 15, /* covered */
  SPR_BLOCK_COUNT = 16
};

enum LedSprite { LED_BLANK = 10, LED_NEGATIVE = 11, LED_COUNT = 12 };

enum ButtonSprite {
  BTN_HAPPY = 0,
  BTN_CAUTION = 1, /* mouse held on field */
  BTN_LOSE = 2,
  BTN_WIN = 3,
  BTN_DOWN = 4, /* the smiley itself pressed */
  BTN_COUNT = 5
};

/* Native (unscaled) sprite dimensions. */
enum { BLOCK_PX = 16, LED_W = 13, LED_H = 23, BUTTON_PX = 24 };

struct Assets {
  SDL_Texture* blocks; /* active set (color or B&W) */
  SDL_Texture* led;
  SDL_Texture* button;
  SDL_Texture* blocks_color;
  SDL_Texture* blocks_bw;
  SDL_Texture* led_color;
  SDL_Texture* led_bw;
  SDL_Texture* button_color;
  SDL_Texture* button_bw;
};

/* Load all sheets from `dir` (the assets/ directory beside the exe). Returns
 * false on failure (textures left null). */
bool assets_load(struct Assets* a, SDL_Renderer* renderer, const char* dir);
void assets_free(struct Assets* a);

/* Point active textures at color or B&W set. */
void assets_set_color(struct Assets* a, bool color);

/* Source sub-rects within the active sheet for a given sprite index. */
SDL_FRect assets_block_rect(int sprite); /* 0..SPR_BLOCK_COUNT-1 */
SDL_FRect assets_led_rect(int digit);    /* 0..LED_COUNT-1 */
SDL_FRect assets_button_rect(int face);  /* 0..BTN_COUNT-1 */

/* Best-effort window icon from winmine.ico in `dir`; ignored on failure. */
void assets_set_window_icon(SDL_Window* window, const char* dir);

#endif /* MINESWEEPER_ASSETS_H */
