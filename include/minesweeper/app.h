/* app.h — application glue + SDL3 callback entry contract. FROZEN CONTRACT
 * (Stream A).
 *
 * AppState is the whole runtime. main.cc defines the SDL_MAIN_USE_CALLBACKS
 * entry points and delegates to these helpers (implemented in app.cc).
 *
 * Post-Stream-A amendment (authorized; see docs/2026-05-23-minesweeper-port-
 * design.md): folded the transient input flags (left/right/chord/quit, menu-bar
 * height, pause tick) into AppState instead of app.cc file-statics.
 */
#ifndef MINESWEEPER_APP_H
#define MINESWEEPER_APP_H

#include <SDL3/SDL.h>
#include <stdint.h>

#include "minesweeper/assets.h"
#include "minesweeper/game.h"
#include "minesweeper/render.h"
#include "minesweeper/types.h"
#include "minesweeper/ui.h"

struct AppState {
  SDL_Window *window;
  SDL_Renderer *renderer;
  struct Assets assets;
  struct Board board;
  struct Settings settings;

  int button_face; /* enum ButtonSprite */
  int press_x;     /* currently held cell, -1 if none */
  int press_y;
  bool pressing_board; /* left button held over a cell */
  bool pressing_face;  /* left button held on smiley */
  bool chord_active;   /* both buttons / middle held */

  bool timer_running;
  uint64_t timer_start_ms;
  int elapsed_sec;
  bool paused; /* minimized / unfocused */

  /* Transient input state (single window). */
  bool left_down;
  bool right_down;
  bool chorded; /* a chord fired; suppress the partner button's action */
  bool want_quit;
  int menu_bar_h;            /* last ImGui main-menu-bar height in px */
  uint64_t pause_started_ms; /* tick when pause began */

  int pending_name_level; /* level awaiting Enter-Name, -1 if none */
  bool show_custom;
  bool show_best;
  bool show_about;
  bool show_name;

  char asset_dir[1024];
  char pref_path[1024];
};

/* Allocate + initialize: SDL, window/renderer, ImGui, config, assets, first
 * game. Returns SDL_APP_CONTINUE on success. */
SDL_AppResult app_init(struct AppState **out, int argc, char **argv);

/* One event. */
SDL_AppResult app_event(struct AppState *s, SDL_Event *event);

/* One frame (continuous vsync redraw). */
SDL_AppResult app_iterate(struct AppState *s);

/* Teardown + save settings. */
void app_quit(struct AppState *s);

#endif /* MINESWEEPER_APP_H */
