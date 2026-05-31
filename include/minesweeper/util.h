/* util.h — small shared helpers (Orthodox C++: free functions, POD, no STL).
 *
 * Cross-module leaf utilities with no SDL/ImGui/mINI dependency, factored out
 * of per-module copies (clamp, path join). */
#ifndef MINESWEEPER_UTIL_H
#define MINESWEEPER_UTIL_H

#include <stddef.h>

/* Clamp v into [lo, hi]. */
int util_clamp(int v, int lo, int hi);

/* Build "dir/name" into buf, collapsing a trailing '/' or '\\' on dir. */
void util_join_path(char* buf, size_t buf_size, const char* dir,
                    const char* name);

#endif /* MINESWEEPER_UTIL_H */
