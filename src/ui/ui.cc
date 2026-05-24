/* ui.cc — STUB (Stream A skeleton). Replaced by Stream B.6 (ImGui menu bar +
 * dialogs). ImGui boundary lines will carry HERESY suppressions. */
#include "minesweeper/ui.h"

#include <string.h>

void ui_actions_clear(struct UiActions *a) {
    memset(a, 0, sizeof *a);
    a->set_difficulty = -1;
    a->set_scale = 0;
}

void ui_apply_theme(void) {}

float ui_menu_bar(const struct Settings *s, struct UiActions *out) {
    (void)s;
    (void)out;
    return 0.0F;
}

void ui_dialogs(const struct Settings *s, struct UiActions *out, bool *show_custom, bool *show_best,
                bool *show_about, bool *show_name, int level_for_name) {
    (void)s;
    (void)out;
    (void)show_custom;
    (void)show_best;
    (void)show_about;
    (void)show_name;
    (void)level_for_name;
}
