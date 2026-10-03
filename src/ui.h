/* Deva's Awesome Adventures - shared pieces of the menus: colours, panels,
 * the "hold the button" gate, item cards and the back hint.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_UI_H
#define DEVA_UI_H

#include "common.h"
#include "gfx.h"

/* the palette of tools/art/pixel.py */
#define COL_INK rgb565(0x3b, 0x1f, 0x4a)     /* outlines, text */
#define COL_DEEP rgb565(0x24, 0x12, 0x2f)    /* shadows, dim */
#define COL_CREAM rgb565(0xff, 0xf4, 0xfa)
#define COL_PINK_L rgb565(0xff, 0xcb, 0xe3)
#define COL_PINK rgb565(0xff, 0x93, 0xc6)
#define COL_HOT rgb565(0xf0, 0x55, 0x9e)
#define COL_LILAC_L rgb565(0xea, 0xdc, 0xfc)
#define COL_LILAC rgb565(0xcb, 0xab, 0xf2)
#define COL_LILAC_D rgb565(0x7c, 0x5f, 0xbb)
#define COL_GOLD rgb565(0xff, 0xd2, 0x3f)
#define COL_GREY rgb565(0xb0, 0xa4, 0xc8)
#define COL_SIL rgb565(0x95, 0x8e, 0xab)     /* silhouettes of what is not won yet */

#define HOLD_FRAMES 120 /* grown-up doors: hold A for 2 seconds */

void ui_backdrop(const char *bg, int dim);           /* background, dimmed 0..256 towards COL_DEEP */
void ui_panel(int x, int y, int w, int h);           /* cream panel with a shadow */
void ui_card(int x, int y, int w, int h, bool sel);  /* a small card, pink frame; gold when selected */
void ui_glow(int x, int y, int w, int h);            /* two rainbow twinkles running round a rectangle */
void ui_title(const char *caps, int y);              /* big capital letters, centred */
void ui_footer(const char *text);                    /* small hint on the bottom line */
void ui_back_hint(void);                             /* the yellow button and an arrow: back */
void ui_silhouette(const sprite_t *s, int x, int y); /* a dark shape: not won yet */
void ui_sticker(const sprite_t *s, int x, int y);    /* a sticker: white border, a little shadow */
/* A ring of little stars around a rectangle, lit one by one while A is held. */
void ui_hold_stars(int x, int y, int w, int h, int held, int total);
void ui_bar(int x, int y, int w, int held, int total); /* the same, as a bar under a line */

#endif
