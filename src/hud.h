/* Deva's Awesome Adventures - drawings of the console (XiFan RF35H) used to show
 * which physical control to press: the whole console with one part glowing,
 * and a close-up of the black cross with one arm lit.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_HUD_H
#define DEVA_HUD_H

#include "common.h"

enum { HUD_NONE, HUD_DPAD, HUD_A, HUD_B };        /* console highlights */
enum { DIR_NONE = -1, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

#define HUD_CONSOLE_W 80
#define HUD_CONSOLE_H 42
#define HUD_DPAD_SIZE 40

/* The console at (x, y), top-left; the highlighted part glows when lit. */
void hud_console(int x, int y, int highlight, bool lit);
void hud_console_zoom(int x, int y, int highlight, bool lit, int k); /* k times bigger */
/* The black cross at (x, y), top-left, with one arm coloured (or DIR_NONE). */
void hud_dpad(int x, int y, int dir);

/* Which part the tutorial voice is talking about right now (HUD_NONE if it
 * is not speaking a tutorial line): "croce nera" then "bottone rosso", or
 * "bottone giallo". */
int hud_tutorial_highlight(void);

#endif
