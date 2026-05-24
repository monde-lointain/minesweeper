/* app.cc — STUB (Stream A skeleton). Replaced by Stream C.0 (full wiring of all
 * modules via the SDL3 callback model, input/timer/draw lifecycle). For the
 * skeleton this just opens a window and clears it. */
#include "minesweeper/app.h"

#include <stdlib.h>
#include <string.h>

#include "minesweeper/config.h"

SDL_AppResult app_init(struct AppState **out, int argc, char **argv) {
    (void)argc;
    (void)argv;
    struct AppState *s = (struct AppState *)calloc(1, sizeof *s);
    if (s == NULL) {
        return SDL_APP_FAILURE;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        free(s);
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Minesweeper", 320, 240, 0, &s->window, &s->renderer)) {
        free(s);
        return SDL_APP_FAILURE;
    }
    config_defaults(&s->settings);
    game_reset(&s->board, BOARD_MIN_W, BOARD_MIN_H, BOARD_MIN_MINES, NULL, NULL);
    s->press_x = -1;
    s->press_y = -1;
    s->pending_name_level = -1;
    *out = s;
    return SDL_APP_CONTINUE;
}

SDL_AppResult app_event(struct AppState *s, SDL_Event *event) {
    (void)s;
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult app_iterate(struct AppState *s) {
    SDL_SetRenderDrawColor(s->renderer, 192, 192, 192, 255);
    SDL_RenderClear(s->renderer);
    SDL_RenderPresent(s->renderer);
    return SDL_APP_CONTINUE;
}

void app_quit(struct AppState *s) {
    if (s == NULL) {
        return;
    }
    if (s->renderer != NULL) {
        SDL_DestroyRenderer(s->renderer);
    }
    if (s->window != NULL) {
        SDL_DestroyWindow(s->window);
    }
    free(s);
}
