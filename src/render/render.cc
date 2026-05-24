/* render.cc — STUB (Stream A skeleton). Replaced by Stream B.5 (board/chrome
 * drawing, layout, hit-testing). */
#include "minesweeper/render.h"

#include <string.h>

void render_compute_layout(const struct Board *b, const struct Settings *s, int menu_bar_h,
                           struct Layout *out) {
    (void)b;
    memset(out, 0, sizeof *out);
    out->scale = s->scale;
    out->menu_bar_h = menu_bar_h;
}

void render_frame(SDL_Renderer *renderer, const struct Assets *a, const struct Board *b,
                  const struct Settings *s, const struct Layout *lay, int button_face, int press_x,
                  int press_y, int elapsed_sec) {
    (void)renderer;
    (void)a;
    (void)b;
    (void)s;
    (void)lay;
    (void)button_face;
    (void)press_x;
    (void)press_y;
    (void)elapsed_sec;
}

bool render_cell_at(const struct Board *b, const struct Layout *lay, float px, float py, int *cx,
                    int *cy) {
    (void)b;
    (void)lay;
    (void)px;
    (void)py;
    (void)cx;
    (void)cy;
    return false;
}

bool render_button_at(const struct Layout *lay, float px, float py) {
    (void)lay;
    (void)px;
    (void)py;
    return false;
}
