/* Deva's Awesome Adventures - software renderer: 320x240 RGB565 framebuffer, sprite atlas,
 * backgrounds, 1-bit transparency blits and number drawing.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_GFX_H
#define DEVA_GFX_H

#include "common.h"

#define GFX_FLIP_H 1
#define GFX_SQUASH_BODY 2 /* gfx_blit_squash: only the body (below the split) gets wider or narrower */

typedef struct {
    int w, h;
    uint16_t *px;  /* RGB565 */
    uint8_t *mask; /* 1 = opaque; NULL for opaque images */
} image_t;

typedef struct {
    const image_t *img;
    int16_t x, y, w, h; /* rectangle inside the atlas */
    int16_t ox, oy;     /* default offset (hero parts) */
} sprite_t;

enum { NUM_NORMAL, NUM_SELECTED, NUM_OFF };

extern uint16_t g_fb[SCREEN_W * SCREEN_H];

bool gfx_init(const char *data_dir);
void gfx_free(void);

/* Lookups: never return NULL (a missing sprite is logged once and drawn as nothing). */
const sprite_t *gfx_sprite(const char *name);
bool gfx_has_sprite(const char *name);
bool gfx_opaque(const sprite_t *s, int i, int j); /* pixel (i, j) of the sprite is drawn */
bool gfx_anchor(const char *name, int *x, int *y);
const image_t *gfx_image(const char *name); /* backgrounds, curtain */
void gfx_prefetch_step(void);               /* decodes one background not needed at the start */

/* Drawing state: clip rectangle and a tint mixed into every blitted pixel
 * (amount 0..256). gfx_reset_state() restores full screen, no tint. */
void gfx_set_clip(int x0, int y0, int x1, int y1);
void gfx_set_tint(uint16_t color, int amount);
void gfx_reset_state(void);
void gfx_get_clip(int *x0, int *y0, int *x1, int *y1); /* (0.13: to draw elsewhere and come back) */
void gfx_get_tint(uint16_t *color, int *amount);
void gfx_get_target(uint16_t **buf, int *w, int *h);

void gfx_draw_bg(const image_t *bg);
void gfx_blit(const sprite_t *s, int x, int y, int flags);
void gfx_blit_solid(const sprite_t *s, int x, int y, uint16_t color, int flags);
void gfx_blit_image(const image_t *img, int x, int y, int flags);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t color);
void gfx_fade(uint16_t color, int amount);   /* mix the frame (inside the clip) towards a colour, 0..256 */
void gfx_shake(int dx, int dy);              /* shift the finished frame: screen shake */
void gfx_blit_image_circle(const image_t *img, int cx, int cy, int r); /* a full-screen image, in a circle */
/* 0.12: a background under the spell of an adventure, with the filters of the maps (grey, dark,
   quiet, still: tools/art/tale*_bg.py), made once and kept until another one is asked for */
enum { SPELL_GREY, SPELL_DARK, SPELL_QUIET, SPELL_STILL };
const image_t *gfx_image_spell(const char *name, int kind);
/* darkness all around a light: the frame outside the circle mixed towards a colour (0..256),
   with a soft edge */
void gfx_fade_outside_circle(int cx, int cy, int r, uint16_t color, int amount);
/* a rounded box mixed towards a colour (0..256): a panel the place shows through (0.12) */
void gfx_fade_round_rect(int x, int y, int w, int h, int r, uint16_t color, int amount);
/* the frame of such a box: an ink line outside, a band of `thick` pixels in colour inside it, its
   upper and left edges a little lighter (light from the upper left) */
void gfx_frame_round(int x, int y, int w, int h, int r, int thick, uint16_t color, uint16_t light, uint16_t ink);
void gfx_blit_scaled(const sprite_t *s, int x, int y, int k, int flags); /* integer zoom k >= 1 */
/* 0.13: squash and stretch for pixel art. The sprite is drawn k times bigger with its feet (the
   middle of its bottom edge) on (fx, fy), dy screen pixels taller (shorter when negative) and dx
   wider (narrower). Nothing is resampled: the rows around one row of the body (split_y, a row of
   the sprite, e.g. the belly) are repeated or hidden, and so are the columns around the middle;
   eyes, mouths and outlines stay sharp. Returns nothing; gfx_squash_y() tells where a point of
   the sprite (row y, k = 1) lands on the screen, relative to the top of the plain drawing. */
void gfx_blit_squash(const sprite_t *s, int fx, int fy, int k, int flags, int split_y, int dy, int dx);
/* the same for a w x h buffer with a key colour (a drawing made with gfx_target: the heroine) */
void gfx_blit_buffer_squash(const uint16_t *buf, int w, int h, uint16_t key, int fx, int fy, int k, int flags,
                            int split_y, int dy, int dx);
int gfx_squash_y(int y, int k, int split_y, int dy); /* screen offset of row y after the squash */
/* any sprite into any rectangle (nearest neighbour): the cards that pop, the stars that land */
void gfx_blit_stretch(const sprite_t *s, int x, int y, int dw, int dh, int flags);
/* a band of light across a sprite already drawn at (x, y): its pixels with i + j/2
   in [pos, pos + w) are lightened towards white (0..256) - the logo shines */
void gfx_shine(const sprite_t *s, int x, int y, int pos, int w, int amount);
/* a w x h buffer squeezed to dw columns, centred on cx (a card that turns round);
   the key colour is transparent */
void gfx_blit_hsqueeze(const uint16_t *buf, int w, int h, uint16_t key, int cx, int y, int dw);
void gfx_number_big(int n, int cx, int cy, int style);
void gfx_number_small(int n, int cx, int cy);
int gfx_number_big_width(int n);
/* Capital letters (A-Z, '?', ' ') in the big font, centred on (cx, cy),
 * zoomed k times; NUM_OFF draws them greyed out. */
void gfx_text_big(const char *s, int cx, int cy, int style, int k);
int gfx_text_big_width(const char *s, int k);

/* The small font (UTF-8, Latin-1 range: Italian accents, « »), 9 px high:
 * y is the top of the line, the baseline is y + 7. Returns the x after the text. */
enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };
int gfx_text(const char *s, int x, int y, uint16_t color, int align);
int gfx_text_shadow(const char *s, int x, int y, uint16_t color, uint16_t shadow, int align);
int gfx_text_width(const char *s);
void gfx_round_rect(int x, int y, int w, int h, uint16_t fill, uint16_t border); /* soft box, 1 px border */

/* Drawing into a buffer instead of the screen (w*h RGB565), until gfx_target(NULL, 0, 0);
 * then the buffer can be turned: its pixel (px, py) lands on (cx, cy), rotated
 * deg degrees clockwise, the key colour left out (the heroine's cartwheel). */
#define GFX_KEY 0xF81F /* magenta, never in the palette */
void gfx_target(uint16_t *buf, int w, int h);
void gfx_blit_rotated(const uint16_t *buf, int w, int h, uint16_t key, int px, int py, int cx, int cy, int deg);

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

#endif
