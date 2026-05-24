/* config.cc — settings persistence backed by mINI (header-only).
 *
 * ALL mINI/STL/std::string use is confined to this file; config.h stays pure
 * POD. Orthodox C++ elsewhere; mINI's API forces a few Modern C++ touches at
 * the call boundary, suppressed inline with HERESY comments where flagged.
 */
#include "minesweeper/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "mini/ini.h"

/* --- helpers (file-local) ------------------------------------------------- */

/* Clamp v into [lo, hi]. */
static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Parse a base-10 int from an INI string; fall back to def if unparseable. */
static int parse_int(const std::string *str, int def) {
    if (str->empty()) return def;
    char *end = NULL;
    long v = strtol(str->c_str(), &end, 10);
    if (end == str->c_str()) return def; /* no digits consumed */
    return (int)v;
}

/* Parse a bool: "1"/"true"/"yes"/"on" -> true; else def for empty, false otherwise. */
static bool parse_bool(const std::string *str, bool def) {
    if (str->empty()) return def;
    const char *c = str->c_str();
    if (strcmp(c, "1") == 0 || strcmp(c, "true") == 0 ||
        strcmp(c, "yes") == 0 || strcmp(c, "on") == 0) {
        return true;
    }
    return false;
}

/* Copy src into a fixed name buffer with bounded ops + guaranteed NUL. */
static void copy_name(char *dst, const std::string *src) {
    strncpy(dst, src->c_str(), SCORE_NAME_MAX - 1);
    dst[SCORE_NAME_MAX - 1] = '\0';
}

/* Read [section][key] into *out_i if present; leave default otherwise. */
static void read_int(mINI::INIStructure *ini, const char *section,
                     const char *key, int *out_i) {
    if (!ini->has(section)) return;
    if (!ini->get(section).has(key)) return;
    std::string v = ini->get(section).get(key);
    *out_i = parse_int(&v, *out_i);
}

static void read_bool(mINI::INIStructure *ini, const char *section,
                      const char *key, bool *out_b) {
    if (!ini->has(section)) return;
    if (!ini->get(section).has(key)) return;
    std::string v = ini->get(section).get(key);
    *out_b = parse_bool(&v, *out_b);
}

static void read_name(mINI::INIStructure *ini, const char *section,
                      const char *key, char *out_name) {
    if (!ini->has(section)) return;
    if (!ini->get(section).has(key)) return;
    std::string v = ini->get(section).get(key);
    if (v.empty()) return; /* keep default name */
    copy_name(out_name, &v);
}

static void set_int(mINI::INIStructure *ini, const char *section,
                    const char *key, int v) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", v);
    (*ini)[section][key] = buf;
}

static void set_bool(mINI::INIStructure *ini, const char *section,
                     const char *key, bool v) {
    (*ini)[section][key] = v ? "1" : "0";
}

/* --- public API ----------------------------------------------------------- */

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
        strncpy(s->best_name[i], "Anonymous", SCORE_NAME_MAX - 1);
        s->best_name[i][SCORE_NAME_MAX - 1] = '\0';
    }
}

int config_load(struct Settings *s, const char *path) {
    config_defaults(s);

    mINI::INIFile file(path);
    mINI::INIStructure ini;
    if (!file.read(ini)) {
        return 1; /* missing/unreadable: defaults already filled */
    }

    read_int(&ini, "game", "difficulty", &s->difficulty);
    read_int(&ini, "game", "custom_w", &s->custom_w);
    read_int(&ini, "game", "custom_h", &s->custom_h);
    read_int(&ini, "game", "custom_mines", &s->custom_mines);
    read_bool(&ini, "game", "sound", &s->sound);
    read_bool(&ini, "game", "marks", &s->marks);
    read_bool(&ini, "game", "color", &s->color);
    read_int(&ini, "game", "scale", &s->scale);
    read_bool(&ini, "game", "menu_visible", &s->menu_visible);

    read_int(&ini, "window", "x", &s->window_x);
    read_int(&ini, "window", "y", &s->window_y);

    read_int(&ini, "scores", "beg_time", &s->best_time[0]);
    read_name(&ini, "scores", "beg_name", s->best_name[0]);
    read_int(&ini, "scores", "int_time", &s->best_time[1]);
    read_name(&ini, "scores", "int_name", s->best_name[1]);
    read_int(&ini, "scores", "exp_time", &s->best_time[2]);
    read_name(&ini, "scores", "exp_name", s->best_name[2]);

    /* Validate / clamp. */
    s->difficulty = clamp_int(s->difficulty, DIFF_BEGINNER, DIFF_CUSTOM);
    s->custom_w = clamp_int(s->custom_w, BOARD_MIN_W, BOARD_MAX_W);
    s->custom_h = clamp_int(s->custom_h, BOARD_MIN_H, BOARD_MAX_H);
    s->custom_mines = clamp_int(s->custom_mines, BOARD_MIN_MINES,
                                s->custom_w * s->custom_h - 1);
    s->scale = clamp_int(s->scale, 1, 4);

    return 0;
}

int config_save(const struct Settings *s, const char *path) {
    mINI::INIFile file(path);
    mINI::INIStructure ini;

    set_int(&ini, "game", "difficulty", s->difficulty);
    set_int(&ini, "game", "custom_w", s->custom_w);
    set_int(&ini, "game", "custom_h", s->custom_h);
    set_int(&ini, "game", "custom_mines", s->custom_mines);
    set_bool(&ini, "game", "sound", s->sound);
    set_bool(&ini, "game", "marks", s->marks);
    set_bool(&ini, "game", "color", s->color);
    set_int(&ini, "game", "scale", s->scale);
    set_bool(&ini, "game", "menu_visible", s->menu_visible);

    set_int(&ini, "window", "x", s->window_x);
    set_int(&ini, "window", "y", s->window_y);

    set_int(&ini, "scores", "beg_time", s->best_time[0]);
    ini["scores"]["beg_name"] = s->best_name[0];
    set_int(&ini, "scores", "int_time", s->best_time[1]);
    ini["scores"]["int_name"] = s->best_name[1];
    set_int(&ini, "scores", "exp_time", s->best_time[2]);
    ini["scores"]["exp_name"] = s->best_name[2];

    if (!file.generate(ini, true)) {
        return 1;
    }
    return 0;
}
