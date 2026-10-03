/* Deva's Awesome Adventures - the little mathematics of movement (0.13): smooth waves instead of
 * square ones, easings, the breathing and the hops of the characters, and the characters that
 * live (they breathe, blink and bounce with their own voice).
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_ANIM_H
#define DEVA_ANIM_H

#include "common.h"

/* amp * sin(2 pi t / period), rounded (a smooth wave) */
int wave(int t, int period, int amp);
/* 0..amp and back, smoothly: amp * (1 - cos(2 pi t / period)) / 2 (what bob() used to do in steps) */
int swing(int t, int period, int amp);
/* a number from a name: characters do not breathe (or blink) all together */
uint32_t anim_seed(const char *s);

/* from -> to in n frames: quick then slow (out), slow-quick-slow (in_out), or overshooting a little
   and settling (out_back, the cards that arrive) */
int ease_out(int t, int n, int from, int to);
int ease_in_out(int t, int n, int from, int to);
int ease_out_back(int t, int n, int from, int to);

/* a hop of n frames, h pixels high, for a drawing hgt pixels tall (k = 1): at frame t the lift
   (pixels above the ground) and the squash of the body (dy taller, dx wider, k = 1): a crouch
   before, stretched while it rises, squashed when it lands */
void hop_shape(int t, int n, int h, int hgt, int *lift, int *dy, int *dx);
/* the same hop, once every `period` frames (t: any clock; seed: so that friends do not hop together) */
void hop_every(int t, uint32_t seed, int period, int n, int h, int hgt, int *lift, int *dy, int *dx);

/* A character of the tale, alive: drawn k times bigger with its feet on (cx, feet). It breathes
 * (its body a pixel taller and back, slowly), blinks (the sprite <name>_bl, if the atlas has it)
 * and, while talk (its voice level, 0..256) is above zero, stretches with every syllable. extra_dy
 * and extra_dx add a squash of the caller's (a hop). Props are not actors: draw them plainly. */
void actor_draw(const char *sprite, int cx, int feet, int k, int flags, int talk, int extra_dy, int extra_dx);
bool actor_is_alive(const char *sprite); /* a character (mago, strega..., mo_*), not a prop */

#endif
