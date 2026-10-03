/* Deva's Awesome Adventures - particles: confetti bursts, sparkles, floating "Z"s,
 * and (0.9.0) twinkles, rainbow bursts, hearts, little cloud puffs, falling stars
 * and the star that flies to the row of stars at the top when an answer is right.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_FX_H
#define DEVA_FX_H

#include "common.h"

void fx_clear(void);
void fx_confetti(int x, int y, int n);
void fx_sparkles(int x, int y, int radius, int n);
void fx_zed(int x, int y);
void fx_twinkle(int x, int y, int delay);            /* one star that twinkles in place */
void fx_trail(int x0, int y0, int x1, int y1, int n); /* twinkles along a line (the highlight moves) */
void fx_burst(int x, int y, int n);                  /* a ring of rainbow stars flying out */
void fx_hearts(int x, int y, int n);                 /* little hearts floating up */
void fx_puff(int x, int y);                          /* "poof": a soft little cloud */
void fx_starfall(int n);                             /* rainbow stars falling from the top (level up) */
/* A star flies from (x0, y0) to (x1, y1) along an arc and lands with a sparkle:
   the row of stars at the top shows it only when it lands (fx_stars_flying). */
void fx_fly_star(int x0, int y0, int x1, int y1);
int fx_stars_flying(void);
int fx_star_landed_age(void); /* frames since the last star landed (large if none) */
void fx_update(void);
void fx_draw(void);

#endif
