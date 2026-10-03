/* Deva's Awesome Adventures - shared pieces of the menus.
 * SPDX-License-Identifier: MIT
 */
#include "ui.h"

#include "amb.h"
#include "game.h"

void ui_backdrop(const char *bg, int dim)
{
    gfx_draw_bg(gfx_image(bg));
    if (dim > 0)
        gfx_fade(COL_DEEP, dim);
    amb_twinkle(36, 26, 284, 186, 8, 60); /* the stars of the stage still twinkle behind */
}

void ui_panel(int x, int y, int w, int h)
{
    gfx_round_rect(x + 2, y + 3, w, h, COL_DEEP, COL_DEEP);
    gfx_round_rect(x, y, w, h, COL_CREAM, COL_INK);
}

void ui_glow(int x, int y, int w, int h)
{
    static const uint8_t RGB[5][3] = {
        {0xff, 0x6d, 0xb0}, {0xff, 0xc8, 0x2e}, {0x4f, 0xd6, 0xc0}, {0x74, 0xb8, 0xff}, {0xb3, 0x76, 0xec}};
    static const sprite_t *TW[2];
    if (!TW[0]) {
        TW[0] = gfx_sprite("tw_2");
        TW[1] = gfx_sprite("tw_3");
    }
    if (w <= 0 || h <= 0)
        return;
    unsigned per = (unsigned)(2 * (w + h));
    for (int k = 0; k < 2; k++) { /* opposite each other, clockwise, two pixels a frame */
        int s = (int)((G.frame * 2u + (unsigned)k * per / 2) % per), px, py;
        if (s < w) {
            px = x + s;
            py = y;
        } else if (s < w + h) {
            px = x + w;
            py = y + s - w;
        } else if (s < 2 * w + h) {
            px = x + w - (s - w - h);
            py = y + h;
        } else {
            px = x;
            py = y + h - (s - 2 * w - h);
        }
        const uint8_t *c = RGB[(G.frame / 10 + (unsigned)k * 2) % 5];
        const sprite_t *tw = TW[((G.frame / 5) + (unsigned)k) & 1];
        gfx_set_tint(rgb565(c[0], c[1], c[2]), 150);
        gfx_blit(tw, px - tw->w / 2, py - tw->h / 2, 0);
    }
    gfx_set_tint(0, 0);
}

void ui_card(int x, int y, int w, int h, bool sel)
{
    if (sel) {
        gfx_round_rect(x - 2, y - 2, w + 4, h + 4, COL_GOLD, COL_INK);
        gfx_round_rect(x, y, w, h, COL_HOT, COL_INK);
        gfx_round_rect(x + 2, y + 2, w - 4, h - 4, 0xffff, COL_HOT);
        ui_glow(x - 3, y - 3, w + 5, h + 5);
    } else {
        gfx_round_rect(x, y, w, h, COL_PINK_L, COL_INK);
        gfx_round_rect(x + 2, y + 2, w - 4, h - 4, COL_CREAM, COL_PINK_L);
    }
}

void ui_title(const char *caps, int y) { gfx_text_big(caps, SCREEN_W / 2, y, NUM_SELECTED, 1); }

void ui_footer(const char *text)
{
    gfx_fill_rect(0, SCREEN_H - 13, SCREEN_W, 13, COL_DEEP);
    gfx_text(text, SCREEN_W / 2, SCREEN_H - 11, COL_LILAC, ALIGN_CENTER);
}

void ui_back_hint(void)
{
    const sprite_t *b = gfx_sprite("btn_b"), *a = gfx_sprite("arrow_left");
    gfx_blit(a, 4, SCREEN_H - 28, 0);
    gfx_blit(b, 28, SCREEN_H - 23, 0);
}

void ui_silhouette(const sprite_t *s, int x, int y) { gfx_blit_solid(s, x, y, COL_SIL, 0); }

void ui_sticker(const sprite_t *s, int x, int y)
{
    /* the shadow, then a white border two pixels wide (a ring of offsets), then the picture */
    static const int8_t RING[12][2] = {{-2, 0}, {2, 0},  {0, -2},  {0, 2},  {-1, -2}, {1, -2},
                                       {-1, 2}, {1, 2},  {-2, -1}, {-2, 1}, {2, -1},  {2, 1}};
    gfx_blit_solid(s, x + 2, y + 3, COL_DEEP, 0);
    for (int i = 0; i < 12; i++)
        gfx_blit_solid(s, x + RING[i][0], y + RING[i][1], 0xffff, 0);
    gfx_blit(s, x, y, 0);
}

void ui_hold_stars(int x, int y, int w, int h, int held, int total)
{
    /* 12 stars: 4 on the top and the bottom edges, 2 on each side, clockwise from the top left */
    enum { N = 12 };
    int px[N], py[N];
    for (int i = 0; i < 4; i++) {
        px[i] = x + i * (w - 11) / 3;
        py[i] = y - 6;
        px[6 + i] = x + (3 - i) * (w - 11) / 3;
        py[6 + i] = y + h - 5;
    }
    px[4] = px[5] = x + w - 5;
    py[4] = y + h / 3 - 5;
    py[5] = y + 2 * h / 3 - 5;
    px[10] = px[11] = x - 6;
    py[10] = y + 2 * h / 3 - 5;
    py[11] = y + h / 3 - 5;
    int lit = total > 0 ? held * N / total : 0;
    for (int i = 0; i < N; i++)
        gfx_blit(gfx_sprite(i < lit ? "lstar_on" : "lstar_off"), px[i], py[i], 0);
}

void ui_bar(int x, int y, int w, int held, int total)
{
    int fill = total > 0 ? clampi(held * (w - 2) / total, 0, w - 2) : 0;
    gfx_fill_rect(x, y, w, 4, COL_INK);
    gfx_fill_rect(x + 1, y + 1, w - 2, 2, COL_LILAC_L);
    gfx_fill_rect(x + 1, y + 1, fill, 2, COL_HOT);
}
