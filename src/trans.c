/* Deva's Awesome Adventures - transitions between scenes.
 *
 * Every style is an "order map": one byte per pixel, the moment (0..255) the
 * pixel of the new scene shows through. Heart, star and circle grow from the
 * middle of the screen (the byte is the size of the shape that reaches the
 * pixel), the rainbow sweeps from the top left, the sparkles are little
 * diamonds, one in every 20x20 square, each starting at its own moment and
 * growing until they meet. The maps are made once at load (five of 75 KB, and
 * the last frame of the old scene, 150 KB). The fade needs no map: old and new
 * are mixed.
 * SPDX-License-Identifier: MIT
 */
#include "trans.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "gfx.h"

#define FRAMES 30 /* half a second */
#define RIM 14    /* width of the shining edge, in map steps */
#define CX 160
#define CY 124
#define NPIX (SCREEN_W * SCREEN_H)
#define ANG 128   /* boundary radius table: angles */
#define CELL 20   /* the squares of the sparkle diamonds */

static uint8_t s_map[TR_FADE - 1][NPIX]; /* from TR_HEART to TR_RAINBOW; the fade has no map */
#define MAP(style) s_map[(style) - 1]
static float s_rad[TR_COUNT][ANG]; /* heart, star, circle: boundary at scale 1, per angle */
static float s_smax[TR_COUNT];     /* the scale that covers the whole screen */
static uint16_t s_old[NPIX];
static struct {
    int style, t;
} T;

static const float PI = 3.14159265f;

static int ang_bin(float a) /* angle (radians, atan2) -> 0..ANG-1 */
{
    int b = (int)floorf((a + PI) / (2 * PI) * ANG + 0.5f);
    return ((b % ANG) + ANG) % ANG;
}

/* the heart: 16 sin^3 t, 13 cos t - 5 cos 2t - 2 cos 3t - cos 4t, tip down, seen
   from a point a little under its middle (from there every ray meets it once) */
static void heart_table(float *r)
{
    for (int i = 0; i < ANG; i++)
        r[i] = 0;
    for (int k = 0; k < 4096; k++) {
        float t = (float)k / 4096.0f * 2 * PI;
        float x = 16 * powf(sinf(t), 3);
        float y = -(13 * cosf(t) - 5 * cosf(2 * t) - 2 * cosf(3 * t) - cosf(4 * t)) - 2.0f;
        int b = ang_bin(atan2f(y, x));
        float d = sqrtf(x * x + y * y) / 17.0f;
        if (d > r[b])
            r[b] = d;
    }
    for (int pass = 0; pass < 2; pass++) /* bins no sample fell into: from the neighbours */
        for (int i = 0; i < ANG; i++)
            if (r[i] <= 0)
                r[i] = (r[(i + ANG - 1) % ANG] + r[(i + 1) % ANG]) / 2;
}

/* the star: five points (one up), inner radius 0.46 */
static void star_table(float *r)
{
    float px[10], py[10];
    for (int k = 0; k < 10; k++) {
        float a = -PI / 2 + k * PI / 5, rr = (k & 1) ? 0.46f : 1.0f;
        px[k] = rr * cosf(a);
        py[k] = rr * sinf(a);
    }
    for (int i = 0; i < ANG; i++) {
        float a = (float)i / ANG * 2 * PI - PI, ux = cosf(a), uy = sinf(a), best = 0;
        for (int k = 0; k < 10; k++) { /* the ray meets one of the ten edges */
            float ax = px[k], ay = py[k], bx = px[(k + 1) % 10], by = py[(k + 1) % 10];
            float ex = bx - ax, ey = by - ay, den = ux * ey - uy * ex;
            if (fabsf(den) < 1e-6f)
                continue;
            float s = (ax * ey - ay * ex) / den, u = (ax * uy - ay * ux) / den;
            if (s > 0 && u >= -1e-4f && u <= 1 + 1e-4f && (best == 0 || s < best))
                best = s;
        }
        r[i] = best > 0 ? best : 0.46f;
    }
}

static void shape_map(int style)
{
    float *r = s_rad[style], smax = 0;
    float *s = malloc(sizeof(float) * NPIX); /* only while the map is made (given back at once) */
    if (!s)
        return; /* no memory: this style is a plain cut */
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            float dx = (float)(x - CX), dy = (float)(y - CY);
            float v = sqrtf(dx * dx + dy * dy) / r[ang_bin(atan2f(dy, dx))];
            s[y * SCREEN_W + x] = v;
            if (v > smax)
                smax = v;
        }
    s_smax[style] = smax;
    for (int i = 0; i < NPIX; i++)
        MAP(style)[i] = (uint8_t)clampi((int)(s[i] / smax * 255.0f), 0, 255);
    free(s);
}

void trans_init(void)
{
    static bool done;
    if (done)
        return;
    done = true;
    heart_table(s_rad[TR_HEART]);
    star_table(s_rad[TR_STAR]);
    for (int i = 0; i < ANG; i++)
        s_rad[TR_CIRCLE][i] = 1.0f;
    shape_map(TR_HEART);
    shape_map(TR_STAR);
    shape_map(TR_CIRCLE);
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            /* the diamond of this 20x20 square: when it starts (0..105), then 7.5 a pixel from its middle */
            uint32_t h = (uint32_t)(x / CELL) * 73856093u ^ (uint32_t)(y / CELL) * 19349663u;
            h ^= h >> 13;
            h *= 0x5bd1e995u;
            h ^= h >> 15;
            int dx = x % CELL - CELL / 2, dy = y % CELL - CELL / 2;
            int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy); /* 0..20 */
            MAP(TR_SPARKLE)[y * SCREEN_W + x] = (uint8_t)clampi((int)(h % 106) + d * 150 / CELL, 0, 255);
            MAP(TR_RAINBOW)[y * SCREEN_W + x] = (uint8_t)((x * 3 + y * 2) * 255 / (SCREEN_W * 3 + SCREEN_H * 2));
        }
}

void trans_begin(int style)
{
    if (style <= TR_NONE || style >= TR_COUNT || !G.cfg.animations)
        return;
    trans_init();
    memcpy(s_old, g_fb, sizeof(s_old));
    T.style = style;
    T.t = 0;
}

bool trans_active(void) { return T.style != TR_NONE; }

static inline uint16_t mix(uint16_t a, uint16_t b, int t) /* a towards b, t 0..256 */
{
    int r = ((a >> 11) * (256 - t) + (b >> 11) * t) >> 8;
    int g = (((a >> 5) & 63) * (256 - t) + ((b >> 5) & 63) * t) >> 8;
    int bl = ((a & 31) * (256 - t) + (b & 31) * t) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static uint16_t rim_colour(int style, int d) /* d: steps behind the edge, 0..RIM-1 */
{
    static const uint8_t RAINBOW[7][3] = {{0xff, 0xff, 0xff}, {0xff, 0x6d, 0x8a}, {0xff, 0xa9, 0x4d}, {0xff, 0xe0, 0x66},
                                          {0x7b, 0xe0, 0x8a}, {0x74, 0xb8, 0xff}, {0xb3, 0x76, 0xec}};
    switch (style) {
    case TR_HEART: return d < 4 ? 0xFFFF : (d < 9 ? rgb565(0xff, 0x93, 0xc6) : rgb565(0xf0, 0x55, 0x9e));
    case TR_STAR: return d < 4 ? 0xFFFF : (d < 9 ? rgb565(0xff, 0xf3, 0xa6) : rgb565(0xff, 0xd2, 0x3f));
    case TR_RAINBOW: {
        const uint8_t *c = RAINBOW[clampi(d / 2, 0, 6)];
        return rgb565(c[0], c[1], c[2]);
    }
    default: return d < 5 ? 0xFFFF : rgb565(0xea, 0xdc, 0xfc);
    }
}

static void edge_twinkles(int style, int p)
{
    static const sprite_t *TW[2];
    if (!TW[0]) {
        TW[0] = gfx_sprite("tw_2");
        TW[1] = gfx_sprite("tw_3");
    }
    if (style == TR_HEART || style == TR_STAR || style == TR_CIRCLE) {
        float s = (float)p / 255.0f * s_smax[style];
        uint16_t tint = style == TR_HEART ? rgb565(0xf0, 0x55, 0x9e) : rgb565(0xf0, 0xa0, 0x30);
        for (int k = 0; k < 14; k++) {
            float a = (float)k / 14 * 2 * PI + (float)T.t * 0.07f;
            float d = s * s_rad[style][ang_bin(a)] + 3.0f; /* just outside the edge */
            int x = CX + (int)(cosf(a) * d), y = CY + (int)(sinf(a) * d);
            const sprite_t *tw = TW[(k + T.t / 3) & 1];
            if (x > -8 && x < SCREEN_W + 8 && y > -8 && y < SCREEN_H + 8) {
                gfx_set_tint(tint, (k & 1) ? 0 : 110);
                gfx_blit(tw, x - tw->w / 2, y - tw->h / 2, 0);
            }
        }
        gfx_set_tint(0, 0);
    } else if (style == TR_RAINBOW) { /* along the sweeping line: 3x + 2y = const */
        int c = p * (SCREEN_W * 3 + SCREEN_H * 2) / 255;
        for (int y = 8; y < SCREEN_H; y += 34) {
            int x = (c - 2 * y) / 3;
            const sprite_t *tw = TW[((y / 34) + T.t / 3) & 1];
            if (x > -8 && x < SCREEN_W + 8)
                gfx_blit(tw, x - tw->w / 2, y - tw->h / 2, 0);
        }
    } else if (style == TR_SPARKLE) { /* on the diamonds that are opening just now */
        for (int k = 0; k < 8; k++) {
            int cx = (int)((T.t * 7 + k * 13) % (SCREEN_W / CELL)), cy = (int)((T.t * 3 + k * 5) % (SCREEN_H / CELL));
            int x = cx * CELL + CELL / 2, y = cy * CELL + CELL / 2, v = MAP(TR_SPARKLE)[y * SCREEN_W + x];
            if (v <= p && v > p - 60) {
                const sprite_t *tw = TW[(k + T.t / 3) & 1];
                gfx_blit(tw, x - tw->w / 2, y - tw->h / 2, 0);
            }
        }
    } else { /* the fade: here and there, where nothing moves */
        for (int k = 0; k < 5; k++) {
            uint32_t h = (uint32_t)(k * 2654435761u) ^ (uint32_t)(T.t / 6) * 40503u;
            const sprite_t *tw = TW[(k + T.t / 3) & 1];
            gfx_blit(tw, (int)(h % (SCREEN_W - 16)) + 4, (int)((h >> 12) % (SCREEN_H - 16)) + 4, 0);
        }
    }
}

void trans_draw(void)
{
    if (T.style == TR_NONE)
        return;
    if (++T.t > FRAMES) {
        T.style = TR_NONE;
        return;
    }
    /* ease in and out: the shape is seen small, grows, and settles; the edge leaves the screen too */
    int u = T.t * 256 / FRAMES, e = u * u / 256 * (768 - 2 * u) / 256;
    if (T.style == TR_FADE) { /* old and new mixed, the old one going away */
        int k = 256 - e;
        for (int i = 0; i < NPIX; i++)
            g_fb[i] = mix(g_fb[i], s_old[i], k);
        edge_twinkles(T.style, e);
        return;
    }
    int rim = T.style == TR_SPARKLE ? RIM / 2 : RIM;
    int p = e * (255 + rim) / 256;
    const uint8_t *m = MAP(T.style);
    uint16_t col[RIM];
    for (int d = 0; d < RIM; d++)
        col[d] = rim_colour(T.style, d);
    for (int i = 0; i < NPIX; i++) {
        int v = m[i];
        if (v > p)
            g_fb[i] = s_old[i];
        else if (v > p - rim)
            g_fb[i] = col[p - v];
    }
    edge_twinkles(T.style, p);
}
