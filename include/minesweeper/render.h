/* render.h — SDL_Renderer board/chrome drawing contract. FROZEN CONTRACT (Stream A).
 *
 * Reproduces winmine grafix.c layout at an integer scale. Win95 chrome palette
 * face 192/highlight 255/shadow 128/dark 0. Board interior is sprite blits; only
 * outer frame + LED panel bevels are hand-drawn. See ~/.../winmine/grafix.{c,h}.
 */
#ifndef MINESWEEPER_RENDER_H
#define MINESWEEPER_RENDER_H

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "minesweeper/assets.h"
#include "minesweeper/game.h"
#include "minesweeper/types.h"

/* Scaled pixel geometry for the current board + scale + menu-bar height. */
struct Layout {
    int scale;
    int menu_bar_h;    /* supplied by app (ImGui frame height; 0 if hidden) */
    int window_w;
    int window_h;
    int grid_x;        /* top-left pixel of the cell grid */
    int grid_y;
    int button_x;      /* smiley rect (BUTTON_PX*scale square) */
    int button_y;
    int bomb_led_x;    /* left 3-digit LED panel */
    int led_y;
    int time_led_x;    /* right 3-digit LED panel */
};

/* Compute window size + element positions. `menu_bar_h` is the ImGui main-menu
 * bar height in pixels (0 when hidden). */
void render_compute_layout(const struct Board *b, const struct Settings *s,
                           int menu_bar_h, struct Layout *out);

/* Draw one frame of board + chrome (call before ImGui render, after clear).
 * `button_face` is enum ButtonSprite. `press_x/press_y` is the currently held
 * cell (-1,-1 if none). `elapsed_sec` and mines-remaining drive the LEDs. */
void render_frame(SDL_Renderer *renderer, const struct Assets *a,
                  const struct Board *b, const struct Settings *s,
                  const struct Layout *lay, int button_face,
                  int press_x, int press_y, int elapsed_sec);

/* Map a window pixel to a cell. Returns false if outside the grid. */
bool render_cell_at(const struct Board *b, const struct Layout *lay,
                    float px, float py, int *cx, int *cy);

/* True if the pixel is within the smiley button rect. */
bool render_button_at(const struct Layout *lay, float px, float py);

#endif /* MINESWEEPER_RENDER_H */
