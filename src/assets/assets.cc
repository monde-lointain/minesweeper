/* assets.cc — STUB (Stream A skeleton). Replaced by Stream B.3 (BMP loading,
 * sub-rect tables, window icon). */
#include "minesweeper/assets.h"

#include <string.h>

bool assets_load(struct Assets *a, SDL_Renderer *renderer, const char *dir) {
    (void)renderer;
    (void)dir;
    memset(a, 0, sizeof *a);
    return false;
}

void assets_free(struct Assets *a) {
    memset(a, 0, sizeof *a);
}

void assets_set_color(struct Assets *a, bool color) {
    (void)a;
    (void)color;
}

SDL_FRect assets_block_rect(int sprite) {
    SDL_FRect r = {0.0F, (float)(sprite * BLOCK_PX), (float)BLOCK_PX, (float)BLOCK_PX};
    return r;
}

SDL_FRect assets_led_rect(int digit) {
    SDL_FRect r = {0.0F, (float)(digit * LED_H), (float)LED_W, (float)LED_H};
    return r;
}

SDL_FRect assets_button_rect(int face) {
    SDL_FRect r = {0.0F, (float)(face * BUTTON_PX), (float)BUTTON_PX, (float)BUTTON_PX};
    return r;
}

void assets_set_window_icon(SDL_Window *window, const char *dir) {
    (void)window;
    (void)dir;
}
