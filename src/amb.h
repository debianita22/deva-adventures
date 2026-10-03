/* Deva's Awesome Adventures - the living details of the backgrounds (0.9.0):
 * stars that twinkle, fireflies, rain, snow, embers, bubbles, drops, petals and
 * hearts, the bulbs of the mirrors, the shine that crosses the glass. Nothing
 * is stored: every detail is a function of its number and of the frame, so a
 * scene only has to call amb_draw() with the name of its background.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_AMB_H
#define DEVA_AMB_H

#include "common.h"

void amb_draw(const char *bg);                                           /* the details of that background */
void amb_bg(const char *bg);                                             /* the background, and its details */
void amb_twinkle(int x0, int y0, int x1, int y1, int n, uint32_t seed); /* stars twinkling in a rectangle */
void amb_petals(int x0, int y0, int x1, int y1, int n, uint32_t seed);  /* pink petals falling */
void amb_hearts(int x0, int y0, int x1, int y1, int n, uint32_t seed);  /* little hearts floating up */

#endif
