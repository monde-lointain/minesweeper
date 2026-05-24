/* render.cc — SDL_Renderer board/chrome drawing (Stream B.5).
 *
 * Reproduces winmine grafix.c layout at integer scale = Settings.scale.
 * Win95 chrome palette: face 192 / highlight 255 / shadow 128 / dark 0. The
 * cell tiles bake their own bevels, so the grid interior is pure sprite blits;
 * only the outer frame + LED/grid panel bevels are hand-drawn here.
 *
 * Sprite-per-state mapping follows winmine rtns.c (ShowBombs/GameOver):
 *   loss: unflagged mine -> SPR_BOMB_DN, wrong flag -> SPR_WRONG,
 *         exploded mine -> SPR_EXPLODE
 *   win:  unflagged mine -> SPR_BOMB_UP
 */
#include "minesweeper/render.h"

#include <string.h>

/* Unscaled winmine metrics (grafix.h). */
enum {
  SP_LEFT = 12,                      /* dxLeftSpace */
  SP_RIGHT = 12,                     /* dxRightSpace */
  SP_TOP = 12,                       /* dyTopSpace */
  SP_BOTTOM = 12,                    /* dyBottomSpace */
  LED_TOP = SP_TOP + 4,              /* dyTopLed = 16 */
  GRID_OFF_X = SP_LEFT,              /* dxGridOff = 12 */
  GRID_OFF_Y = LED_TOP + LED_H + 16, /* dyGridOff = 55 */
  LEFT_BOMB = SP_LEFT + 5,           /* dxLeftBomb = 17 */
  RIGHT_TIME = SP_RIGHT + 5          /* dxRightTime = 17 */
};

/* Win95 chrome colors. */
enum {
  FACE_R = 192,
  FACE_G = 192,
  FACE_B = 192,
  HI = 255, /* highlight (top/left) */
  SH = 128, /* shadow (bottom/right) */
  DK = 0    /* dark */
};

static void set_color(SDL_Renderer *r, int c) {
  SDL_SetRenderDrawColor(r, (Uint8)c, (Uint8)c, (Uint8)c, 255);
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h) {
  SDL_FRect rc;
  rc.x = (float)x;
  rc.y = (float)y;
  rc.w = (float)w;
  rc.h = (float)h;
  SDL_RenderFillRect(r, &rc);
}

/* Beveled border of `width` px around the rect (x,y,w,h), in scaled pixels.
 * `raised` true -> highlight top/left, shadow bottom/right (raised look).
 * raised false -> shadow top/left, highlight bottom/right (recessed look). */
static void draw_bevel(SDL_Renderer *r, int x, int y, int w, int h, int width,
                       int scale, bool raised) {
  int i;
  int tl = raised ? HI : SH;
  int br = raised ? SH : HI;
  int t = width * scale;
  for (i = 0; i < t; ++i) {
    /* top edge */
    set_color(r, tl);
    fill(r, x + i, y + i, w - 2 * i, 1);
    /* left edge */
    fill(r, x + i, y + i, 1, h - 2 * i);
    /* bottom edge */
    set_color(r, br);
    fill(r, x + i, y + h - 1 - i, w - 2 * i, 1);
    /* right edge */
    fill(r, x + w - 1 - i, y + i, 1, h - 2 * i);
  }
}

void render_compute_layout(const struct Board *b, const struct Settings *s,
                           int menu_bar_h, struct Layout *out) {
  int scale;
  int win_w_u; /* unscaled client width  */
  int win_h_u; /* unscaled client height */

  memset(out, 0, sizeof *out);
  scale = s->scale;
  if (scale < 1) {
    scale = 1;
  }
  out->scale = scale;
  out->menu_bar_h = menu_bar_h;

  /* dxWindow = dxBlk*W + dxGridOff + dxRightSpace; dyWindow likewise. */
  win_w_u = BLOCK_PX * b->width + GRID_OFF_X + SP_RIGHT;
  win_h_u = BLOCK_PX * b->height + GRID_OFF_Y + SP_BOTTOM;

  out->window_w = win_w_u * scale;
  out->window_h = win_h_u * scale + menu_bar_h;

  out->grid_x = GRID_OFF_X * scale;
  out->grid_y = GRID_OFF_Y * scale + menu_bar_h;

  out->led_y = LED_TOP * scale + menu_bar_h;
  out->bomb_led_x = LEFT_BOMB * scale;
  /* Right LED panel: dxWindow - (dxRightTime + 3*dxLed). */
  out->time_led_x = (win_w_u - (RIGHT_TIME + 3 * LED_W)) * scale;

  /* Smiley centered: (dxWindow - dxButton)/2, at dyTopLed. */
  out->button_x = ((win_w_u - BUTTON_PX) / 2) * scale;
  out->button_y = LED_TOP * scale + menu_bar_h;
}

/* Blit one scaled sprite. */
static void blit(SDL_Renderer *r, SDL_Texture *tex, SDL_FRect src, int dx,
                 int dy, int dw, int dh) {
  SDL_FRect dst;
  dst.x = (float)dx;
  dst.y = (float)dy;
  dst.w = (float)dw;
  dst.h = (float)dh;
  SDL_RenderTexture(r, tex, &src, &dst);
}

/* Draw a 3-digit LED panel at left pixel x, top pixel y. `value` may be
 * negative: leftmost digit becomes LED_NEGATIVE, magnitude shown mod 100. */
static void draw_led3(SDL_Renderer *r, SDL_Texture *tex, int x, int y,
                      int scale, int value) {
  int d0; /* leftmost (hundreds / sign) */
  int d1;
  int d2;
  int mag;
  int lw = LED_W * scale;
  int lh = LED_H * scale;

  if (value < 0) {
    d0 = LED_NEGATIVE;
    mag = (-value) % 100;
    d1 = mag / 10;
    d2 = mag % 10;
  } else {
    if (value > 999) {
      value = 999;
    }
    d0 = (value / 100) % 10;
    d1 = (value / 10) % 10;
    d2 = value % 10;
  }

  blit(r, tex, assets_led_rect(d0), x, y, lw, lh);
  blit(r, tex, assets_led_rect(d1), x + lw, y, lw, lh);
  blit(r, tex, assets_led_rect(d2), x + 2 * lw, y, lw, lh);
}

/* Choose the sprite index for one cell given the game state. */
static int cell_sprite(const struct Board *b, int x, int y, int press_x,
                       int press_y) {
  int idx = game_index(b, x, y);
  const struct Cell *c = &b->cells[idx];
  bool lost = (b->status == GAME_LOST);
  bool won = (b->status == GAME_WON);

  if (c->revealed) {
    if (c->mine) {
      /* The clicked mine on loss. */
      return c->exploded ? SPR_EXPLODE : SPR_BOMB_DN;
    }
    if (c->adjacent == 0) {
      return SPR_BLANK;
    }
    return (int)c->adjacent; /* SPR_1..SPR_8 == n */
  }

  /* Covered cell. */
  if (lost) {
    if (c->mine) {
      /* Unflagged mine revealed as a bomb; flagged mine keeps the flag. */
      if (c->flag == FLAG_MINE) {
        return SPR_BOMB_UP;
      }
      return SPR_BOMB_DN;
    }
    if (c->flag == FLAG_MINE) {
      return SPR_WRONG; /* wrongly flagged */
    }
  } else if (won) {
    /* On win, mines are shown flagged. */
    if (c->mine) {
      return SPR_BOMB_UP;
    }
  }

  /* In-progress covered states. */
  if (c->flag == FLAG_MINE) {
    return SPR_BOMB_UP;
  }
  if (c->flag == FLAG_QUESTION) {
    /* Held-down question mark shows the pressed look. */
    if (x == press_x && y == press_y) {
      return SPR_GUESS_DN;
    }
    return SPR_GUESS_UP;
  }
  /* Plain covered: pressed cell shows the depressed blank tile. */
  if (x == press_x && y == press_y) {
    return SPR_BLANK;
  }
  return SPR_BLANK_UP;
}

void render_frame(SDL_Renderer *renderer, const struct Assets *a,
                  const struct Board *b, const struct Settings *s,
                  const struct Layout *lay, int button_face, int press_x,
                  int press_y, int elapsed_sec) {
  int scale = lay->scale;
  int top = lay->menu_bar_h;
  int x;
  int y;
  int cell = BLOCK_PX * scale;
  int led_w = LED_W * scale;
  int led_h = LED_H * scale;
  int btn = BUTTON_PX * scale;
  int gw = b->width * cell;  /* grid pixel width */
  int gh = b->height * cell; /* grid pixel height */

  (void)s;

  /* 1. Win95 chrome. ----------------------------------------------------- */
  /* Background face fill (over the whole board content area below the menu). */
  set_color(renderer, FACE_R);
  fill(renderer, 0, top, lay->window_w, lay->window_h - top);

  /* Outer frame: raised 3px bevel around the whole client. */
  draw_bevel(renderer, 0, top, lay->window_w, lay->window_h - top, 3, scale,
             true);

  /* Recessed 3px bevel around the grid. */
  draw_bevel(renderer, lay->grid_x - 3 * scale, lay->grid_y - 3 * scale,
             gw + 6 * scale, gh + 6 * scale, 3, scale, false);

  /* Recessed 2px bevel around the status panel (between top space and LEDs). */
  {
    int px = lay->grid_x - 3 * scale;
    int py = (SP_TOP - 3) * scale + top;
    int pw = gw + 6 * scale;
    /* height: from (SP_TOP-3) down to (dyTopLed+dyLed+dyBottomSpace-6).
     * = LED_TOP + LED_H + (SP_BOTTOM-6) - (SP_TOP-3); computed numerically
     * to avoid cross-enum arithmetic. */
    int panel_bottom = (int)LED_TOP + (int)LED_H + (SP_BOTTOM - 6);
    int ph = (panel_bottom - (SP_TOP - 3)) * scale;
    draw_bevel(renderer, px, py, pw, ph, 2, scale, false);
  }

  /* Recessed 1px bevel around the bomb-count LED panel. */
  draw_bevel(renderer, lay->bomb_led_x - scale, lay->led_y - scale,
             3 * led_w + 2 * scale, led_h + 2 * scale, 1, scale, false);

  /* Recessed 1px bevel around the time LED panel. */
  draw_bevel(renderer, lay->time_led_x - scale, lay->led_y - scale,
             3 * led_w + 2 * scale, led_h + 2 * scale, 1, scale, false);

  /* Raised 1px bevel around the smiley button. */
  draw_bevel(renderer, lay->button_x - scale, lay->button_y - scale,
             btn + 2 * scale, btn + 2 * scale, 1, scale, true);

  /* 2. Bomb-count LED (left). ------------------------------------------- */
  draw_led3(renderer, a->led, lay->bomb_led_x, lay->led_y, scale,
            game_mines_remaining(b));

  /* 3. Time LED (right). ------------------------------------------------- */
  {
    int t = elapsed_sec;
    if (t < 0) {
      t = 0;
    }
    if (t > 999) {
      t = 999;
    }
    draw_led3(renderer, a->led, lay->time_led_x, lay->led_y, scale, t);
  }

  /* 4. Smiley button. ---------------------------------------------------- */
  {
    int face = button_face;
    if (face < 0 || face >= BTN_COUNT) {
      face = BTN_HAPPY;
    }
    blit(renderer, a->button, assets_button_rect(face), lay->button_x,
         lay->button_y, btn, btn);
  }

  /* 5. Grid cells. ------------------------------------------------------- */
  for (y = 0; y < b->height; ++y) {
    for (x = 0; x < b->width; ++x) {
      int spr = cell_sprite(b, x, y, press_x, press_y);
      int dx = lay->grid_x + x * cell;
      int dy = lay->grid_y + y * cell;
      blit(renderer, a->blocks, assets_block_rect(spr), dx, dy, cell, cell);
    }
  }
}

bool render_cell_at(const struct Board *b, const struct Layout *lay, float px,
                    float py, int *cx, int *cy) {
  int cell = BLOCK_PX * lay->scale;
  float fx = px - (float)lay->grid_x;
  float fy = py - (float)lay->grid_y;
  int gx;
  int gy;

  if (fx < 0.0f || fy < 0.0f) {
    return false;
  }
  gx = (int)(fx / (float)cell);
  gy = (int)(fy / (float)cell);
  if (gx < 0 || gx >= b->width || gy < 0 || gy >= b->height) {
    return false;
  }
  *cx = gx;
  *cy = gy;
  return true;
}

bool render_button_at(const struct Layout *lay, float px, float py) {
  int btn = BUTTON_PX * lay->scale;
  float bx = (float)lay->button_x;
  float by = (float)lay->button_y;
  return (px >= bx && px < bx + (float)btn && py >= by && py < by + (float)btn);
}
