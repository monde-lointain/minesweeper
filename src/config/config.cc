/* config.cc — settings persistence backed by mINI (header-only).
 *
 * ALL mINI/STL/std::string use is confined to this file; config.h stays pure
 * POD. Orthodox C++ elsewhere; mINI's API forces a few Modern C++ touches at
 * the call boundary, suppressed inline with HERESY comments where flagged.
 */
#include "minesweeper/config.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "minesweeper/util.h"
#include "mini/ini.h"

/* --- helpers (file-local) ------------------------------------------------- */

/* Parse a base-10 int from an INI string; fall back to def if unparseable. */
static int parse_int(const std::string* str, int def) {
  if (str->empty()) return def;
  char* end = NULL;
  long v = strtol(str->c_str(), &end, 10);
  if (end == str->c_str()) return def; /* no digits consumed */
  return (int)v;
}

/* Parse a bool: "1"/"true"/"yes"/"on" -> true; else def for empty, false
 * otherwise. */
static bool parse_bool(const std::string* str, bool def) {
  if (str->empty()) return def;
  const char* c = str->c_str();
  if (strcmp(c, "1") == 0 || strcmp(c, "true") == 0 || strcmp(c, "yes") == 0 ||
      strcmp(c, "on") == 0) {
    return true;
  }
  return false;
}

/* Copy src into a fixed name buffer with bounded ops + guaranteed NUL. */
static void copy_name(char* dst, const std::string* src) {
  strncpy(dst, src->c_str(), SCORE_NAME_MAX - 1);
  dst[SCORE_NAME_MAX - 1] = '\0';
}

/* Fetch [section][key] into *out; return false (leaving *out untouched) when
 * the section or key is absent. Shared guard for the read_* helpers. */
static bool ini_get(mINI::INIStructure* ini, const char* section,
                    const char* key, std::string* out) {
  if (!ini->has(section)) return false;
  if (!ini->get(section).has(key)) return false;
  *out = ini->get(section).get(key);
  return true;
}

/* Read [section][key] into *out_i if present; leave default otherwise. */
static void read_int(mINI::INIStructure* ini, const char* section,
                     const char* key, int* out_i) {
  std::string v;
  if (!ini_get(ini, section, key, &v)) return;
  *out_i = parse_int(&v, *out_i);
}

static void read_bool(mINI::INIStructure* ini, const char* section,
                      const char* key, bool* out_b) {
  std::string v;
  if (!ini_get(ini, section, key, &v)) return;
  *out_b = parse_bool(&v, *out_b);
}

static void read_name(mINI::INIStructure* ini, const char* section,
                      const char* key, char* out_name) {
  std::string v;
  if (!ini_get(ini, section, key, &v)) return;
  if (v.empty()) return; /* keep default name */
  copy_name(out_name, &v);
}

static void set_int(mINI::INIStructure* ini, const char* section,
                    const char* key, int v) {
  char buf[32];
  snprintf(buf, sizeof buf, "%d", v);
  (*ini)[section][key] = buf;
}

static void set_bool(mINI::INIStructure* ini, const char* section,
                     const char* key, bool v) {
  (*ini)[section][key] = v ? "1" : "0";
}

/* --- scalar field table --------------------------------------------------- */

/* One row per scalar Settings field. Adding a setting = one row here (plus a
 * clamp line in config_load if it needs range validation). Both load and save
 * iterate this table, so the two directions can no longer drift apart. Scores
 * (arrays of int + name) don't fit the scalar shape and are handled separately
 * below. `off` locates the field via offsetof — valid because Settings is POD.
 */
enum { CFG_INT, CFG_BOOL };

struct ConfigField {
  const char* section;
  const char* key;
  int type;
  size_t off;
};

static const struct ConfigField k_fields[] = {
    {"game", "difficulty", CFG_INT, offsetof(struct Settings, difficulty)},
    {"game", "custom_w", CFG_INT, offsetof(struct Settings, custom_w)},
    {"game", "custom_h", CFG_INT, offsetof(struct Settings, custom_h)},
    {"game", "custom_mines", CFG_INT, offsetof(struct Settings, custom_mines)},
    {"game", "sound", CFG_BOOL, offsetof(struct Settings, sound)},
    {"game", "marks", CFG_BOOL, offsetof(struct Settings, marks)},
    {"game", "color", CFG_BOOL, offsetof(struct Settings, color)},
    {"game", "scale", CFG_INT, offsetof(struct Settings, scale)},
    {"game", "menu_visible", CFG_BOOL, offsetof(struct Settings, menu_visible)},
    {"window", "x", CFG_INT, offsetof(struct Settings, window_x)},
    {"window", "y", CFG_INT, offsetof(struct Settings, window_y)},
};

enum { CONFIG_FIELD_COUNT = (int)(sizeof k_fields / sizeof k_fields[0]) };

/* Score key prefixes by level (beg/int/exp). Keys are "<prefix>_time" and
 * "<prefix>_name" under the [scores] section. */
static const char* k_score_prefix[LEVEL_COUNT] = {"beg", "int", "exp"};

/* --- public API ----------------------------------------------------------- */

void config_defaults(struct Settings* s) {
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
    strncpy(s->best_name[i], "Anonymous", SCORE_NAME_MAX - 1);
    s->best_name[i][SCORE_NAME_MAX - 1] = '\0';
  }
}

int config_load(struct Settings* s, const char* path) {
  config_defaults(s);

  mINI::INIFile file(path);
  mINI::INIStructure ini;
  if (!file.read(ini)) {
    return 1; /* missing/unreadable: defaults already filled */
  }

  for (int i = 0; i < CONFIG_FIELD_COUNT; ++i) {
    const struct ConfigField* f = &k_fields[i];
    char* field = (char*)s + f->off;
    if (f->type == CFG_BOOL) {
      read_bool(&ini, f->section, f->key, (bool*)field);
    } else {
      read_int(&ini, f->section, f->key, (int*)field);
    }
  }

  for (int i = 0; i < LEVEL_COUNT; ++i) {
    char key[16];
    snprintf(key, sizeof key, "%s_time", k_score_prefix[i]);
    read_int(&ini, "scores", key, &s->best_time[i]);
    snprintf(key, sizeof key, "%s_name", k_score_prefix[i]);
    read_name(&ini, "scores", key, s->best_name[i]);
  }

  /* Validate / clamp. */
  s->difficulty = util_clamp(s->difficulty, DIFF_BEGINNER, DIFF_CUSTOM);
  s->custom_w = util_clamp(s->custom_w, BOARD_MIN_W, BOARD_MAX_W);
  s->custom_h = util_clamp(s->custom_h, BOARD_MIN_H, BOARD_MAX_H);
  s->custom_mines = util_clamp(s->custom_mines, BOARD_MIN_MINES,
                               s->custom_w * s->custom_h - 1);
  s->scale = util_clamp(s->scale, 1, 4);

  return 0;
}

int config_save(const struct Settings* s, const char* path) {
  mINI::INIFile file(path);
  mINI::INIStructure ini;

  for (int i = 0; i < CONFIG_FIELD_COUNT; ++i) {
    const struct ConfigField* f = &k_fields[i];
    const char* field = (const char*)s + f->off;
    if (f->type == CFG_BOOL) {
      set_bool(&ini, f->section, f->key, *(const bool*)field);
    } else {
      set_int(&ini, f->section, f->key, *(const int*)field);
    }
  }

  for (int i = 0; i < LEVEL_COUNT; ++i) {
    char key[16];
    snprintf(key, sizeof key, "%s_time", k_score_prefix[i]);
    set_int(&ini, "scores", key, s->best_time[i]);
    snprintf(key, sizeof key, "%s_name", k_score_prefix[i]);
    ini["scores"][key] = s->best_name[i];
  }

  if (!file.generate(ini, true)) {
    return 1;
  }
  return 0;
}
