/* config.h — settings persistence contract. FROZEN CONTRACT (Stream A).
 *
 * Implementation confines ALL mINI/STL/exception use to config.cc; this
 * interface is pure POD + errno-style codes. SDL-free: the caller supplies the
 * file path (app resolves it via SDL_GetPrefPath; tests pass a temp path).
 */
#ifndef MINESWEEPER_CONFIG_H
#define MINESWEEPER_CONFIG_H

#include "minesweeper/types.h"

/* Populate `s` with built-in defaults (Beginner, color, scale 2, sound on,
 * marks on, menu visible, best times 999/"Anonymous"). */
void config_defaults(struct Settings *s);

/* Load settings from INI at `path`. Missing/unreadable file -> defaults filled,
 * returns nonzero; full success returns 0. Missing keys keep their defaults. */
int config_load(struct Settings *s, const char *path);

/* Write settings to INI at `path`. Returns 0 on success, nonzero on I/O error. */
int config_save(const struct Settings *s, const char *path);

#endif /* MINESWEEPER_CONFIG_H */
