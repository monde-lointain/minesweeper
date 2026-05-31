/* ui.h — ImGui menu bar + dialogs contract. FROZEN CONTRACT (Stream A).
 *
 * ImGui's C++ API is confined to ui.cc/app.cc with per-line HERESY
 * suppressions. ui fills a POD UiActions each frame; app applies the requested
 * actions. Dialog layouts mirror winmine pref.dlg (Custom field, Best Times,
 * Enter Name, About).
 */
#ifndef MINESWEEPER_UI_H
#define MINESWEEPER_UI_H

#include <stdbool.h>

#include "minesweeper/types.h"

/* Requested actions for this frame; app reads and applies, then discards. */
struct UiActions {
  bool new_game;
  bool quit;
  int set_difficulty; /* -1 = none, else enum Difficulty */
  bool toggle_marks;
  bool toggle_color;
  bool toggle_sound;
  int set_scale;    /* 0 = none, else 1..4 */
  bool toggle_menu; /* hide/show menu bar */

  bool open_custom; /* request opening dialogs */
  bool open_best;
  bool open_about;

  bool custom_applied; /* Custom dialog OK */
  int custom_w;
  int custom_h;
  int custom_mines;

  bool best_reset; /* Best Times "Reset" */

  bool name_entered; /* Enter Name OK */
  char name[SCORE_NAME_MAX];
};

/* Modal-dialog state owned by the app (was ui.cc function-statics; see design
 * doc amendment 2026-05-31). Holds each dialog's visibility plus the edit
 * buffers that must persist across frames while a dialog is open. Zero-init is
 * the closed/unseeded state. */
struct DialogState {
  bool show_custom;
  bool show_best;
  bool show_about;
  bool show_name;

  int custom_w; /* Custom-field edit buffers (seeded from Settings on open). */
  int custom_h;
  int custom_mines;
  bool custom_seeded;

  char name_buf[SCORE_NAME_MAX]; /* Enter-Name edit buffer. */
  bool name_seeded;
};

/* Zero all action fields. */
void ui_actions_clear(struct UiActions* a);

/* Apply the light/Win95 theme once at startup. */
void ui_apply_theme(void);

/* Draw the main menu bar (when settings->menu_visible). Returns its pixel
 * height (0 if hidden). Fills `out` with any triggered actions. */
float ui_menu_bar(const struct Settings* s, struct UiActions* out);

/* Drive the modal dialogs. `ds` (app-owned) carries visibility + edit buffers;
 * ui clears a dialog's show_* when it closes and writes results into `out`. */
void ui_dialogs(const struct Settings* s, struct UiActions* out,
                struct DialogState* ds);

#endif /* MINESWEEPER_UI_H */
