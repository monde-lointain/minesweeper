/* render_smoke_test.cc — headless regression guard for sprite rendering.
 *
 * Catches the class of bug where textures load but are unrenderable (e.g. the
 * SDL3 indexed-BMP issue: SDL_CreateTextureFromSurface on a palettized surface
 * yields a texture that fails to blit with "Texture doesn't have a palette").
 * Renders a frame offscreen and asserts the smiley actually paints yellow.
 */
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include "minesweeper/assets.h"
#include "minesweeper/config.h"
#include "minesweeper/game.h"
#include "minesweeper/render.h"

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

/* Average RGB + lit-pixel count over a sprite cell rendered at (px,py,w,h).
 * Uses only color/coverage, so it is immune to any vertical-flip in readback.
 */
static void cell_stats(SDL_Surface* s, int px, int py, int w, int h, int* r_out,
                       int* g_out, int* b_out, int* lit_out) {
  long r = 0;
  long g = 0;
  long b = 0;
  int lit = 0;
  int n = 0;
  for (int y = 2; y < h - 2; ++y) {
    for (int x = 2; x < w - 2; ++x) {
      Uint8 rr = 0;
      Uint8 gg = 0;
      Uint8 bb = 0;
      Uint8 aa = 0;
      SDL_ReadSurfacePixel(s, px + x, py + y, &rr, &gg, &bb, &aa);
      r += rr;
      g += gg;
      b += bb;
      if ((int)rr > 130 && (int)rr > (int)gg + 40) {
        ++lit; /* bright-red LED segment pixel */
      }
      ++n;
    }
  }
  *r_out = (int)(r / n);
  *g_out = (int)(g / n);
  *b_out = (int)(b / n);
  *lit_out = lit;
}

/* Regression: the original sheets store cells in REVERSE vertical order, so
 * assets_*_rect must index from the bottom. A reversal renders SPR_1 (blue "1")
 * as the red flag and turns LED digit 0 into the sparse negative bar. Checked
 * by color and lit-pixel count, which survive readback orientation quirks. */
TEST(RenderSmoke, SpriteIndexOrderNotReversed) {
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
  /* Force the CPU software renderer: the offscreen driver's default GPU
   * renderer can't get a context on headless macOS CI runners. */
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    GTEST_SKIP() << "no SDL video: " << SDL_GetError();
  }
  SDL_Window* win = nullptr;
  SDL_Renderer* ren = nullptr;
  ASSERT_TRUE(SDL_CreateWindowAndRenderer("d", 200, 100, 0, &win, &ren))
      << SDL_GetError();
  SDL_SetRenderLogicalPresentation(ren, 200, 100,
                                   SDL_LOGICAL_PRESENTATION_DISABLED);
  Assets a;
  ASSERT_TRUE(assets_load(&a, ren, ASSET_DIR));

  SDL_SetRenderDrawColor(ren, 180, 180, 180, 255);
  SDL_RenderClear(ren);
  SDL_FRect s1 = assets_block_rect(SPR_1);
  SDL_FRect d1 = {0, 0, 16, 16};
  SDL_RenderTexture(ren, a.blocks, &s1, &d1);
  SDL_FRect sf = assets_block_rect(SPR_BOMB_UP);
  SDL_FRect df = {20, 0, 16, 16};
  SDL_RenderTexture(ren, a.blocks, &sf, &df);
  SDL_FRect l0 = assets_led_rect(0);
  SDL_FRect dl0 = {40, 0, 13, 23};
  SDL_RenderTexture(ren, a.led, &l0, &dl0);
  SDL_FRect lneg = assets_led_rect(LED_NEGATIVE);
  SDL_FRect dln = {60, 0, 13, 23};
  SDL_RenderTexture(ren, a.led, &lneg, &dln);

  SDL_Surface* surf = SDL_RenderReadPixels(ren, nullptr);
  ASSERT_NE(surf, nullptr) << SDL_GetError();

  int r = 0;
  int g = 0;
  int b = 0;
  int lit = 0;
  /* SPR_1 must be the blue "1", not the red flag. */
  cell_stats(surf, 0, 0, 16, 16, &r, &g, &b, &lit);
  EXPECT_GT(b, r + 20) << "SPR_1 renders red (flag) -> blocks sheet reversed";
  /* SPR_BOMB_UP must be the red flag, not the blue "1". */
  cell_stats(surf, 20, 0, 16, 16, &r, &g, &b, &lit);
  EXPECT_GT(r, b + 20)
      << "SPR_BOMB_UP renders blue (1) -> blocks sheet reversed";
  /* LED digit 0 lights 6 segments; the negative glyph lights ~1. */
  int lit0 = 0;
  int litneg = 0;
  cell_stats(surf, 40, 0, 13, 23, &r, &g, &b, &lit0);
  cell_stats(surf, 60, 0, 13, 23, &r, &g, &b, &litneg);
  EXPECT_GT(lit0, litneg * 2) << "LED digit 0 too sparse -> LED sheet reversed";

  SDL_DestroySurface(surf);
  assets_free(&a);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

TEST(RenderSmoke, SpritesActuallyPaint) {
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
  /* Force the CPU software renderer: the offscreen driver's default GPU
   * renderer can't get a context on headless macOS CI runners. */
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    GTEST_SKIP() << "no SDL video: " << SDL_GetError();
  }
  SDL_Window* win = nullptr;
  SDL_Renderer* ren = nullptr;
  ASSERT_TRUE(SDL_CreateWindowAndRenderer("t", 400, 500, 0, &win, &ren))
      << SDL_GetError();

  Assets a;
  ASSERT_TRUE(assets_load(&a, ren, ASSET_DIR)) << "assets_load failed";

  Settings s;
  config_defaults(&s);
  Board b;
  game_reset(&b, 9, 9, 10, nullptr);
  Layout lay;
  render_compute_layout(&b, &s, 0, &lay);

  SDL_SetRenderDrawColor(ren, 192, 192, 192, 255);
  SDL_RenderClear(ren);
  FrameView view = {BTN_HAPPY, -1, -1, 0};
  render_frame(ren, &a, &b, &lay, &view);

  SDL_Surface* surf = SDL_RenderReadPixels(ren, nullptr);
  ASSERT_NE(surf, nullptr) << SDL_GetError();

  /* Smiley centre must be the happy face's yellow, not the gray background. */
  int sx = lay.button_x + (BUTTON_PX * lay.scale) / 2;
  int sy = lay.button_y + (BUTTON_PX * lay.scale) / 2;
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 bl = 0;
  Uint8 al = 0;
  ASSERT_TRUE(SDL_ReadSurfacePixel(surf, sx, sy, &r, &g, &bl, &al))
      << SDL_GetError();
  EXPECT_GT(r, 200) << "smiley not painted (red chan)";
  EXPECT_GT(g, 200) << "smiley not painted (green chan)";
  EXPECT_LT(bl, 120) << "smiley not painted (blue chan)";

  SDL_DestroySurface(surf);
  assets_free(&a);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}
