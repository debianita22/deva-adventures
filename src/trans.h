/* Deva's Awesome Adventures - transitions between scenes (0.9.0): the new scene
 * opens over the last frame of the old one inside a growing heart (the games), a
 * star (the tale, the map, the duels, the start of the show), a circle
 * (goodnight), a rainbow sweep (the dressing room after a round), little
 * diamonds popping up everywhere like sparkles (the menus), with a shining edge
 * and twinkles on it; the pictures of a tale melt into each other (fade).
 * Purely a picture: the logic of the new scene starts at once, as before.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_TRANS_H
#define DEVA_TRANS_H

#include "common.h"

enum { TR_NONE, TR_HEART, TR_STAR, TR_CIRCLE, TR_SPARKLE, TR_RAINBOW, TR_FADE, TR_COUNT };

void trans_init(void);         /* the order maps of the shapes (once, at load) */
void trans_begin(int style);   /* keeps the frame on the screen as the old picture */
void trans_draw(void);         /* over the new scene, every frame while it lasts */
bool trans_active(void);

#endif
