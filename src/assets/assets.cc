/* assets.cc — BMP sprite-sheet loading, sub-rect tables, window icon. */
#include "minesweeper/assets.h"

#include <stdio.h>
#include <string.h>

/* Build "dir/name" into buf, collapsing a trailing slash on dir. */
static void assets_join_path(char *buf, size_t buf_size, const char *dir,
                             const char *name) {
  size_t len = strlen(dir);
  if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\')) {
    snprintf(buf, buf_size, "%s%s", dir, name);
  } else {
    snprintf(buf, buf_size, "%s/%s", dir, name);
  }
}

/* Load one BMP into a texture with nearest-neighbor scaling. Returns null on
 * failure (diagnostic printed). */
static SDL_Texture *assets_load_sheet(SDL_Renderer *renderer, const char *dir,
                                      const char *name) {
  char path[1024];
  assets_join_path(path, sizeof path, dir, name);

  SDL_Surface *surf = SDL_LoadBMP(path);
  if (surf == NULL) {
    printf("assets: SDL_LoadBMP(%s) failed: %s\n", path, SDL_GetError());
    return NULL;
  }

  /* The original sheets are palettized (INDEX4/INDEX1). SDL3 does NOT auto-
   * convert indexed surfaces when creating a texture; the resulting indexed
   * texture is unrenderable ("Texture doesn't have a palette"). Convert to a
   * packed RGBA format first so the texture renders on every backend. */
  SDL_Surface *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(surf);
  if (rgba == NULL) {
    printf("assets: SDL_ConvertSurface(%s) failed: %s\n", path, SDL_GetError());
    return NULL;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, rgba);
  SDL_DestroySurface(rgba);
  if (tex == NULL) {
    printf("assets: SDL_CreateTextureFromSurface(%s) failed: %s\n", path,
           SDL_GetError());
    return NULL;
  }

  SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
  return tex;
}

bool assets_load(struct Assets *a, SDL_Renderer *renderer, const char *dir) {
  memset(a, 0, sizeof *a);

  a->blocks_color = assets_load_sheet(renderer, dir, "blocks.bmp");
  a->blocks_bw = assets_load_sheet(renderer, dir, "blocksbw.bmp");
  a->led_color = assets_load_sheet(renderer, dir, "led.bmp");
  a->led_bw = assets_load_sheet(renderer, dir, "ledbw.bmp");
  a->button_color = assets_load_sheet(renderer, dir, "button.bmp");
  a->button_bw = assets_load_sheet(renderer, dir, "buttonbw.bmp");

  if (a->blocks_color == NULL || a->blocks_bw == NULL || a->led_color == NULL ||
      a->led_bw == NULL || a->button_color == NULL || a->button_bw == NULL) {
    assets_free(a);
    return false;
  }

  assets_set_color(a, true);
  return true;
}

void assets_free(struct Assets *a) {
  if (a->blocks_color != NULL) {
    SDL_DestroyTexture(a->blocks_color);
  }
  if (a->blocks_bw != NULL) {
    SDL_DestroyTexture(a->blocks_bw);
  }
  if (a->led_color != NULL) {
    SDL_DestroyTexture(a->led_color);
  }
  if (a->led_bw != NULL) {
    SDL_DestroyTexture(a->led_bw);
  }
  if (a->button_color != NULL) {
    SDL_DestroyTexture(a->button_color);
  }
  if (a->button_bw != NULL) {
    SDL_DestroyTexture(a->button_bw);
  }
  memset(a, 0, sizeof *a);
}

void assets_set_color(struct Assets *a, bool color) {
  if (color) {
    a->blocks = a->blocks_color;
    a->led = a->led_color;
    a->button = a->button_color;
  } else {
    a->blocks = a->blocks_bw;
    a->led = a->led_bw;
    a->button = a->button_bw;
  }
}

SDL_FRect assets_block_rect(int sprite) {
  SDL_FRect r = {0.0F, (float)(sprite * BLOCK_PX), (float)BLOCK_PX,
                 (float)BLOCK_PX};
  return r;
}

SDL_FRect assets_led_rect(int digit) {
  SDL_FRect r = {0.0F, (float)(digit * LED_H), (float)LED_W, (float)LED_H};
  return r;
}

SDL_FRect assets_button_rect(int face) {
  SDL_FRect r = {0.0F, (float)(face * BUTTON_PX), (float)BUTTON_PX,
                 (float)BUTTON_PX};
  return r;
}

void assets_set_window_icon(SDL_Window *window, const char *dir) {
  /* Best-effort: core SDL_LoadBMP cannot parse .ico; attempt it anyway and
   * no-op gracefully on failure. Never fail the app. */
  char path[1024];
  assets_join_path(path, sizeof path, dir, "winmine.ico");

  SDL_Surface *surf = SDL_LoadBMP(path);
  if (surf == NULL) {
    return;
  }

  SDL_SetWindowIcon(window, surf);
  SDL_DestroySurface(surf);
}
