/* config.cc — STUB (Stream A skeleton). Replaced by Stream B.2 with the mINI
 * wrapper. All STL/mINI use will be confined to this file. */
#include "minesweeper/config.h"

#include <string.h>

void config_defaults(struct Settings *s) {
    memset(s, 0, sizeof *s);
    s->difficulty = DIFF_BEGINNER;
    s->custom_w = BOARD_MIN_W;
    s->custom_h = BOARD_MIN_H;
    s->custom_mines = BOARD_MIN_MINES;
    s->sound = true;
    s->marks = true;
    s->color = true;
    s->scale = 2;
    s->menu_visible = true;
    s->window_x = -1;
    s->window_y = -1;
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        s->best_time[i] = 999;
        strcpy(s->best_name[i], "Anonymous");
    }
}

int config_load(struct Settings *s, const char *path) {
    (void)path;
    config_defaults(s);
    return 1;
}

int config_save(const struct Settings *s, const char *path) {
    (void)s;
    (void)path;
    return 0;
}
