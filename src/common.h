/* Deva's Awesome Adventures - shared definitions.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_COMMON_H
#define DEVA_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define FPS 60
#define GAME_VERSION "1.2.0"
#define AUDIO_RATE 44100
#define SAMPLES_PER_FRAME (AUDIO_RATE / FPS) /* 735 stereo frames */

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } log_level_t;

/* Routed to the frontend log interface (libretro.c). */
void log_msg(log_level_t level, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/* Small deterministic RNG (xorshift32). */
void rng_seed(uint32_t seed);
uint32_t rng_next(void);
int rng_range(int lo, int hi); /* inclusive */
/* Two random numbers drawn in a fixed order. C leaves open the order in which the arguments of a call,
   the items of an initializer or the two sides of an assignment are evaluated: rng_range() twice in one
   expression gave GCC (x86) and clang (the aarch64 build) different games from the same seed (0.15). */
static inline void rng_pair(int lo1, int hi1, int lo2, int hi2, int *a, int *b)
{
    *a = rng_range(lo1, hi1);
    *b = rng_range(lo2, hi2);
}

static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int absi(int v) { return v < 0 ? -v : v; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }

/* Directories resolved at load time (libretro.c). */
const char *dir_data(void); /* <system>/deva_adventures */
const char *dir_save(void); /* frontend save directory */
void path_join(char *out, size_t n, const char *dir, const char *name);

/* Frontend services used by the game. */
void frontend_rumble(int strength, int frames);
void frontend_shutdown(void);

#endif
