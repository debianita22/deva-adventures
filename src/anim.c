/* Deva's Awesome Adventures - the little mathematics of movement (0.13).
 * SPDX-License-Identifier: MIT
 */
#include "anim.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "gfx.h"

#define TAU 6.28318530718

int wave(int t, int period, int amp)
{
    if (period <= 0)
        return 0;
    double a = TAU * (double)(t % period) / (double)period;
    return (int)lround(amp * sin(a));
}

int swing(int t, int period, int amp)
{
    if (period <= 0)
        return 0;
    double a = TAU * (double)(t % period) / (double)period;
    return (int)lround(amp * (1.0 - cos(a)) * 0.5);
}

uint32_t anim_seed(const char *s)
{
    uint32_t h = 2166136261u;
    while (s && *s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h ^ (h >> 15);
}

static double unit(int t, int n)
{
    if (n <= 0 || t >= n)
        return 1.0;
    return t <= 0 ? 0.0 : (double)t / (double)n;
}

int ease_out(int t, int n, int from, int to)
{
    double u = 1.0 - unit(t, n);
    return from + (int)lround((to - from) * (1.0 - u * u * u));
}

int ease_in_out(int t, int n, int from, int to)
{
    double u = unit(t, n);
    double e = u < 0.5 ? 4 * u * u * u : 1.0 - pow(-2.0 * u + 2.0, 3) / 2.0;
    return from + (int)lround((to - from) * e);
}

int ease_out_back(int t, int n, int from, int to)
{
    const double c1 = 1.70158, c3 = c1 + 1.0;
    double u = unit(t, n) - 1.0;
    return from + (int)lround((to - from) * (1.0 + c3 * u * u * u + c1 * u * u));
}

void hop_shape(int t, int n, int h, int hgt, int *lift, int *dy, int *dx)
{
    /* the first fifth: a crouch; then the arc; then 8 frames of landing */
    int pre = n / 5, air = n - pre, sq = imax(2, hgt / 10); /* a tenth of the height */
    *lift = 0;
    *dy = 0;
    *dx = 0;
    if (t < 0 || t >= n + 8)
        return;
    if (t < pre) { /* getting ready: lower and wider */
        int d = ease_out(t, pre, 0, sq);
        *dy = -d;
        *dx = d;
    } else if (t < n) { /* in the air: stretched when fast (taking off, coming down), round at the top */
        int u = t - pre;
        *lift = (4 * h * u * (air - u)) / imax(1, air * air);
        int v = absi(air - 2 * u) * sq / imax(1, air); /* 0 at the top, sq at both ends */
        *dy = v;
        *dx = -v / 2;
    } else { /* landing: squashed, and back */
        int u = t - n, d = u < 2 ? sq : ease_out(u - 2, 6, sq, 0);
        *dy = -d;
        *dx = d;
    }
}

void hop_every(int t, uint32_t seed, int period, int n, int h, int hgt, int *lift, int *dy, int *dx)
{
    hop_shape((t + (int)(seed % (uint32_t)imax(1, period))) % imax(1, period), n, h, hgt, lift, dy, dx);
}

/* ------------------------------------------------------------------ the characters of the tale */

static const char *const ALIVE[] = {"mago", "strega", "stregone", "orco", "re_", "mo_"};

bool actor_is_alive(const char *sprite)
{
    if (!sprite)
        return false;
    for (int i = 0; i < ARRAY_LEN(ALIVE); i++)
        if (!strncmp(sprite, ALIVE[i], strlen(ALIVE[i])))
            return true;
    return false;
}

void actor_draw(const char *sprite, int cx, int feet, int k, int flags, int talk, int extra_dy, int extra_dx)
{
    char base[48], name[56];
    snprintf(base, sizeof(base), "%s", sprite);
    size_t n = strlen(base);
    if (n > 2 && base[n - 2] == '_' && base[n - 1] == 'p') /* "mago_p": the mouth open, same face */
        base[n - 2] = 0;
    else if (n > 3 && base[n - 3] == '_' && base[n - 1] == 'p') /* "strega_bp" -> "strega_b" */
        base[n - 1] = 0;
    uint32_t seed = anim_seed(base) + (uint32_t)cx * 2654435761u;
    int t = (int)G.frame + (int)(seed % 997);
    const sprite_t *s = gfx_sprite(sprite);
    /* blinking: a few frames now and then, every character at its own pace */
    int period = 190 + (int)(seed % 140);
    if ((t % period) < 7) {
        snprintf(name, sizeof(name), "%s_bl", base);
        if (gfx_has_sprite(name))
            s = gfx_sprite(name);
    }
    /* breathing: a pixel (k pixels) taller and back, every 2-3 seconds */
    int breath = swing(t, 140 + (int)(seed % 50), k);
    int dy = breath + extra_dy, dx = extra_dx;
    if (talk > 0) { /* the voice: taller and a little narrower on every loud syllable */
        dy += talk * 3 * k / 256;
        dx -= talk * k / 256;
    }
    int split = s->h * 62 / 100; /* the belly: the face never stretches */
    gfx_blit_squash(s, cx, feet, k, flags, split, dy, dx);
}
