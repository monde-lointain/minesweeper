/* render_smoke_test.cc — headless regression guard for sprite rendering.
 *
 * Catches the class of bug where textures load but are unrenderable (e.g. the
 * SDL3 indexed-BMP issue: SDL_CreateTextureFromSurface on a palettized surface
 * yields a texture that fails to blit with "Texture doesn't have a palette").
 * Renders a frame offscreen and asserts the smiley actually paints yellow.
 */
#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include "minesweeper/assets.h"
#include "minesweeper/config.h"
#include "minesweeper/game.h"
#include "minesweeper/render.h"

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

TEST(RenderSmoke, SpritesActuallyPaint) {
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    GTEST_SKIP() << "no SDL video: " << SDL_GetError();
  }
  SDL_Window *win = nullptr;
  SDL_Renderer *ren = nullptr;
  ASSERT_TRUE(SDL_CreateWindowAndRenderer("t", 400, 500, 0, &win, &ren));

  Assets a;
  ASSERT_TRUE(assets_load(&a, ren, ASSET_DIR)) << "assets_load failed";

  Settings s;
  config_defaults(&s);
  Board b;
  game_reset(&b, 9, 9, 10, nullptr, nullptr);
  Layout lay;
  render_compute_layout(&b, &s, 0, &lay);

  SDL_SetRenderDrawColor(ren, 192, 192, 192, 255);
  SDL_RenderClear(ren);
  render_frame(ren, &a, &b, &s, &lay, BTN_HAPPY, -1, -1, 0);

  SDL_Surface *surf = SDL_RenderReadPixels(ren, nullptr);
  ASSERT_NE(surf, nullptr) << SDL_GetError();

  /* Smiley centre must be the happy face's yellow, not the gray background. */
  int sx = lay.button_x + (BUTTON_PX * lay.scale) / 2;
  int sy = lay.button_y + (BUTTON_PX * lay.scale) / 2;
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 bl = 0;
  Uint8 al = 0;
  ASSERT_TRUE(SDL_ReadSurfacePixel(surf, sx, sy, &r, &g, &bl, &al)) << SDL_GetError();
  EXPECT_GT(r, 200) << "smiley not painted (red chan)";
  EXPECT_GT(g, 200) << "smiley not painted (green chan)";
  EXPECT_LT(bl, 120) << "smiley not painted (blue chan)";

  SDL_DestroySurface(surf);
  assets_free(&a);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
