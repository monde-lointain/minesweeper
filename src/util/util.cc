/* util.cc — shared leaf helpers. See util.h. */
#include "minesweeper/util.h"

#include <stdio.h>
#include <string.h>

int util_clamp(int v, int lo, int hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

int util_min(int a, int b) {
  return a < b ? a : b;
}

void util_join_path(char* buf, size_t buf_size, const char* dir,
                    const char* name) {
  size_t len = strlen(dir);
  if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\')) {
    snprintf(buf, buf_size, "%s%s", dir, name);
  } else {
    snprintf(buf, buf_size, "%s/%s", dir, name);
  }
}
