/* Deva's Awesome Adventures - the living details of the backgrounds.
 * SPDX-License-Identifier: MIT
 */
#include "amb.h"

#include <stdio.h>
#include <string.h>

#include "game.h"
#include "gfx.h"

static const sprite_t *TW[4], *GLOW_LIME, *GLOW_GOLD, *GLOW_EMBER, *GLOW_CYAN, *FLAKE, *FLAKE_S, *BUBBLE, *PETAL,
    *HEART_S, *NOTINA[3], *BIRD[2], *BFLY[3][2], *CLOUD[3];

static void sprites(void)
{
    if (TW[0])
        return;
    TW[0] = gfx_sprite("tw_0");
    TW[1] = gfx_sprite("tw_1");
    TW[2] = gfx_sprite("tw_2");
    TW[3] = gfx_sprite("tw_3");
    GLOW_LIME = gfx_sprite("glow_lime");
    GLOW_GOLD = gfx_sprite("glow_gold");
    GLOW_EMBER = gfx_sprite("glow_ember");
    GLOW_CYAN = gfx_sprite("glow_cyan");
    FLAKE = gfx_sprite("flake");
    FLAKE_S = gfx_sprite("flake_s");
    BUBBLE = gfx_sprite("bubble");
    PETAL = gfx_sprite("petal");
    HEART_S = gfx_sprite("heart_s");
    NOTINA[0] = gfx_sprite("notina_0");
    NOTINA[1] = gfx_sprite("notina_1");
    NOTINA[2] = gfx_sprite("notina_2");
    BIRD[0] = gfx_sprite("uccello_0");
    BIRD[1] = gfx_sprite("uccello_1");
    CLOUD[0] = gfx_sprite("nuvola_0");
    CLOUD[1] = gfx_sprite("nuvola_1");
    CLOUD[2] = gfx_sprite("nuvola_2");
    static const char *const COL[3] = {"rosa", "gialla", "azzurra"};
    char name[32];
    for (int c = 0; c < 3; c++)
        for (int f = 0; f < 2; f++) {
            snprintf(name, sizeof(name), "farfalla_%s_%d", COL[c], f);
            BFLY[c][f] = gfx_sprite(name);
        }
}

static uint32_t hsh(uint32_t a, uint32_t b)
{
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}

/* sin(i * 2pi / 64) * 64 */
static const int8_t SIN64[64] = {0,   6,   12,  19,  24,  30,  36,  41,  45,  49,  53,  56,  59,  61,  63,  64,
                                 64,  64,  63,  61,  59,  56,  53,  49,  45,  41,  36,  30,  24,  19,  12,  6,
                                 0,   -6,  -12, -19, -24, -30, -36, -41, -45, -49, -53, -56, -59, -61, -63, -64,
                                 -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36, -30, -24, -19, -12, -6};

static void centred(const sprite_t *s, int x, int y) { gfx_blit(s, x - s->w / 2, y - s->h / 2, 0); }

static uint32_t now(void) { return G.frame; }

/* ------------------------------------------------------------------ layers */

/* a star now and then lights up (grows, shines, fades) and then elsewhere */
static void twinkle_sized(int x0, int y0, int x1, int y1, int n, uint32_t seed, int big)
{
    static const uint8_t TWF[14] = {0, 1, 1, 2, 2, 3, 3, 3, 2, 2, 1, 1, 0, 0};
    int w = x1 - x0, h = y1 - y0;
    if (w <= 0 || h <= 0)
        return;
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i);
        uint32_t period = 90 + k % 150, t = now() + (k >> 8) % period;
        uint32_t local = t % period, cycle = t / period;
        if (local >= 14)
            continue;
        uint32_t q = hsh(seed + cycle, (uint32_t)i * 7u + 3u);
        int f = TWF[local];
        if (!big && f > 1) /* small ones: only the little sizes */
            f = 1;
        centred(TW[f], x0 + (int)(q % (uint32_t)w), y0 + (int)((q >> 11) % (uint32_t)h));
    }
}

void amb_twinkle(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    if (!G.cfg.animations)
        return;
    sprites();
    twinkle_sized(x0, y0, x1, y1, n, seed, 1);
}

/* glowing dots wandering in slow loops, blinking now and then */
static void fireflies(int x0, int y0, int x1, int y1, int n, uint32_t seed, const sprite_t *glow)
{
    int w = x1 - x0, h = y1 - y0;
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i);
        uint32_t t = now() + k % 997;
        if ((t / 9) % 7 == 0) /* a blink */
            continue;
        int x = x0 + (int)(k % (uint32_t)w) + SIN64[(t / 3 + k) & 63] * 12 / 64;
        int y = y0 + (int)((k >> 10) % (uint32_t)h) + SIN64[(t / 4 + (k >> 5) + 16) & 63] * 7 / 64;
        centred(glow, x, y);
    }
}

/* things that go up (bubbles, embers) or down (snow, petals), swaying */
static void drift(int x0, int y0, int x1, int y1, int n, uint32_t seed, const sprite_t *s, int speed, bool up,
                  int sway, int flip_every)
{
    int w = x1 - x0, h = y1 - y0;
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i);
        int v = speed + (int)(k % 5); /* sixteenths of a pixel per frame */
        int d = (int)(((now() * (uint32_t)v) / 16 + (k >> 7)) % (uint32_t)h);
        int y = up ? y1 - d : y0 + d;
        int x = x0 + (int)((k >> 3) % (uint32_t)w) + SIN64[(now() / 2 + (k >> 13)) & 63] * sway / 64;
        int flags = flip_every && ((now() / (uint32_t)flip_every + i) & 1) ? GFX_FLIP_H : 0;
        gfx_blit(s, x - s->w / 2, y - s->h / 2, flags);
    }
}

static void rain(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    int w = x1 - x0, h = y1 - y0;
    uint16_t c1 = rgb565(0x9a, 0xa6, 0xcc), c2 = rgb565(0x6c, 0x78, 0xa0);
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i);
        int v = 5 + (int)(k % 3); /* pixels per frame */
        int len = 4 + (int)(k >> 5) % 5;
        int y = y0 + (int)((now() * (uint32_t)v + (k >> 8)) % (uint32_t)h);
        int x = x0 + (int)(((k >> 3) + now() * 2u) % (uint32_t)w);
        for (int j = 0; j < len; j++)
            gfx_fill_rect(x - j / 2, y + j, 1, 1, (k & 1) ? c1 : c2);
    }
}

/* a drop swells at the tip of a stalactite, falls, and leaves a tiny splash */
static void drips(const int (*tips)[2], int n, int floor_y)
{
    uint16_t c = rgb565(0x9a, 0xd8, 0xff);
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(77, (uint32_t)i), period = 150 + k % 120, t = (now() + k % period) % period;
        int x = tips[i][0], y = tips[i][1];
        if (t < 40) { /* swelling */
            gfx_fill_rect(x, y + 1, 1, t < 20 ? 1 : 2, c);
        } else {
            int fall = (int)(t - 40), dy = fall * fall / 12;
            if (y + dy < floor_y)
                gfx_fill_rect(x, y + 2 + dy, 1, 2, c);
            else if (y + dy < floor_y + 30)
                centred(TW[0], x, floor_y);
        }
    }
}

/* the shine of a mirror: a band of light crossing the glass every few seconds */
static void mirror_shine(int x0, int y0, int x1, int y1, int period)
{
    int t = (int)(now() % (uint32_t)period), span = (x1 - x0) + (y1 - y0);
    if (t >= 60)
        return;
    int pos = x0 - (y1 - y0) + t * (span + 20) / 60;
    uint16_t c = rgb565(0xf6, 0xfb, 0xff);
    for (int y = y0; y < y1; y++) {
        int x = pos + (y1 - y);
        int a = x < x0 ? x0 : x, b = x + 4 > x1 ? x1 : x + 4;
        if (a < b)
            gfx_fill_rect(a, y, b - a, 1, c);
    }
}

/* the bulbs of a mirror: now one, now another shines brighter */
static void bulbs(const int (*b)[2], int n, uint32_t seed)
{
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i), period = 70 + k % 90, t = (now() + k) % period;
        if (t < 10)
            centred(TW[t < 3 || t > 7 ? 1 : 2], b[i][0], b[i][1]);
    }
}

void amb_petals(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    if (!G.cfg.animations)
        return;
    sprites();
    drift(x0, y0, x1, y1, n, seed, PETAL, 9, false, 14, 40);
}

void amb_hearts(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    if (!G.cfg.animations)
        return;
    sprites();
    drift(x0, y0, x1, y1, n, seed, HEART_S, 6, true, 10, 0);
}

/* the valley sings again (0.10): little notes of three colours float up, swaying */
static void notes(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    for (int c = 0; c < 3; c++)
        drift(x0, y0, x1, y1, n, seed + (uint32_t)c, NOTINA[c], 5, true, 10, 0);
}

/* the bulbs of the big top: two strings hanging in arcs, and the ring round the star */
static void circus_bulbs(void)
{
    static int b[52][2], n;
    if (!n) {
        static const int ROW[2][2] = {{34, 14}, {58, 16}}; /* y at the ends, sag in the middle (tools/art/tale3_bg.py) */
        for (int r = 0; r < 2; r++)
            for (int x = 8; x < 320; x += 16) {
                b[n][0] = x;
                b[n][1] = ROW[r][0] + 2 + ROW[r][1] * 4 * x * (320 - x) / (320 * 320);
                n++;
            }
        for (int a = 0; a < 12; a++) { /* 22 px around (160, 108), every 30 degrees */
            static const int8_t C[12] = {22, 19, 11, 0, -11, -19, -22, -19, -11, 0, 11, 19};
            b[n][0] = 160 + C[a];
            b[n][1] = 108 + C[(a + 9) % 12];
            n++;
        }
    }
    bulbs(b, n, 44);
}

/* the lights of the merry-go-round (tools/art/tale4_bg.py, carousel(160, 178, 74, 150)) and of the
   bowling alley (the bar over the lanes) */
static void carousel_bulbs(void)
{
    static int b[32][2], n;
    if (!n)
        for (int x = 87; x < 234 && n < 32; x += 5) {
            b[n][0] = x;
            b[n][1] = 67;
            n++;
        }
    bulbs(b, n, 64);
}

static void bowling_bulbs(void)
{
    static int b[27][2], n;
    if (!n)
        for (int x = 6; x < 320 && n < 27; x += 12) {
            b[n][0] = x;
            b[n][1] = 96;
            n++;
        }
    bulbs(b, n, 65);
}

/* 0.13: a little flock crossing the sky now and then (one to three birds, flapping, a little up and down) */
static void birds(int x0, int x1, int y0, int y1, uint32_t seed)
{
    uint32_t clock = now() + seed * 131u, period = 1500 + seed % 600;
    int t = (int)(clock % period), span = x1 - x0 + 60, dur = span * 10 / 7; /* 0.7 px a frame */
    if (t >= dur + 40)
        return;
    uint32_t k = hsh(seed, clock / period);
    bool rtl = k & 1;
    int n = 1 + (int)((k >> 3) % 3), y = y0 + (int)((k >> 8) % (uint32_t)imax(1, y1 - y0));
    for (int i = 0; i < n; i++) {
        int along = t * 7 / 10 - i * 13;
        int x = rtl ? x1 + 30 - along : x0 - 30 + along;
        int yy = y + (i == 1 ? -5 : (i == 2 ? 5 : 0)) + SIN64[(t / 2 + i * 9) & 63] * 3 / 64;
        const sprite_t *s = BIRD[((t / 6) + i) & 1];
        gfx_blit(s, x - s->w / 2, yy, 0);
    }
}

/* butterflies wandering over the grass, flapping */
/* little clouds drifting slowly across a day sky, from the left, round and round (6-12 px a second) */
static void clouds(int y0, int y1, int n, uint32_t seed)
{
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i);
        const sprite_t *s = CLOUD[k % 3];
        uint32_t every = 5 + (k >> 4) % 6, span = (uint32_t)(SCREEN_W + s->w + 60);
        int x = (int)((now() / every + (k >> 9)) % span) - s->w - 30;
        int y = y0 + (int)((k >> 6) % (uint32_t)imax(1, y1 - y0));
        gfx_blit(s, x, y, 0);
    }
}

static void butterflies(int x0, int y0, int x1, int y1, int n, uint32_t seed)
{
    int w = imax(1, x1 - x0), h = imax(1, y1 - y0);
    for (int i = 0; i < n; i++) {
        uint32_t k = hsh(seed, (uint32_t)i), t = now() + (k >> 4) % 1000;
        int x = x0 + (int)(k % (uint32_t)w) + SIN64[(t / 3 + (k >> 6)) & 63] * 16 / 64 + SIN64[(t / 7) & 63] * 8 / 64;
        int y = y0 + (int)((k >> 10) % (uint32_t)h) + SIN64[(t / 2 + 16 + (k >> 9)) & 63] * 5 / 64;
        const sprite_t *s = BFLY[(k >> 20) % 3][(t / 4) & 1];
        gfx_blit(s, x - s->w / 2, y - s->h / 2, 0);
    }
}

/* now and then a shooting star, down to the left, with its tail */
static void shooting_star(int x0, int y0, int x1, int y1, uint32_t seed)
{
    uint32_t clock = now() + seed * 977u, period = 700 + seed % 500;
    int t = (int)(clock % period);
    if (t >= 24)
        return;
    uint32_t k = hsh(seed, clock / period);
    int x = x0 + 30 + (int)(k % (uint32_t)imax(1, x1 - x0 - 30)) - t * 3;
    int y = y0 + (int)((k >> 9) % (uint32_t)imax(1, (y1 - y0) / 2)) + t * 3 / 2;
    static const uint16_t TAIL[6] = {0xffff, 0xff9c, 0xef5d, 0xce59, 0xad95, 0x8cd3};
    for (int j = 1; j <= 6 && j <= t + 1; j++) /* the tail: whiter near the head */
        gfx_fill_rect(x + j * 3 - 1, y - j * 3 / 2, 2, 1, TAIL[j - 1]);
    centred(TW[t < 18 ? (t & 4 ? 2 : 1) : 0], x, y);
}

/* the bulbs round the sign of "Il mio nome" (tools/art/games6.py, marquee_bulbs: every 12 px round the
   panel (80, 28)-(302, 166)): a light runs round the frame, clockwise, like the signs of the theatres */
static void marquee_bulbs(void)
{
    static int b[64][2], n;
    if (!n) {
        for (int x = 86; x <= 290; x += 12) /* top, left to right */
            b[n][0] = x, b[n][1] = 31, n++;
        for (int y = 46; y <= 154; y += 12) /* right side, down */
            b[n][0] = 299, b[n][1] = y, n++;
        for (int x = 290; x >= 86; x -= 12) /* bottom, right to left */
            b[n][0] = x, b[n][1] = 163, n++;
        for (int y = 154; y >= 46; y -= 12) /* left side, up */
            b[n][0] = 83, b[n][1] = y, n++;
    }
    int head = (int)(now() / 5);
    for (int i = 0; i < n; i++)
        if ((i + n - head % n) % 7 == 0)
            centred(TW[1], b[i][0], b[i][1]);
}

/* ------------------------------------------------------------------ the places */

void amb_bg(const char *bg)
{
    gfx_draw_bg(gfx_image(bg));
    amb_draw(bg);
}

void amb_draw(const char *bg)
{
    if (!bg || !G.cfg.animations) /* "animazioni = 0": the backgrounds stand still, as before 0.9.0 */
        return;
    sprites();
    if (!strcmp(bg, "bg_palco") || !strcmp(bg, "bg_notte")) {
        twinkle_sized(36, 26, 284, 186, !strcmp(bg, "bg_notte") ? 14 : 10, 11, 1);
        if (!strcmp(bg, "bg_notte"))
            shooting_star(60, 24, 280, 110, 90);
    } else if (!strcmp(bg, "bg_conta")) { /* the strip of sky beside the panel */
        twinkle_sized(34, 30, 78, 188, 4, 12, 1);
    } else if (!strcmp(bg, "bg_bosco")) {
        twinkle_sized(0, 4, SCREEN_W, 100, 12, 13, 1);
        fireflies(8, 112, 312, 196, 7, 14, GLOW_LIME);
    } else if (!strcmp(bg, "bg_palude")) {
        twinkle_sized(0, 2, SCREEN_W, 60, 6, 15, 1);
        fireflies(24, 96, 296, 190, 7, 16, GLOW_LIME);
    } else if (!strcmp(bg, "bg_grotta")) {
        static const int TIPS[6][2] = {{74, 46}, {98, 24}, {120, 34}, {196, 38}, {238, 48}, {262, 30}};
        twinkle_sized(0, 4, SCREEN_W, 26, 8, 17, 0); /* glow-worms */
        twinkle_sized(152, 30, 200, 190, 5, 18, 0); /* dust in the beam */
        drips(TIPS, 6, 204);
        static const int GEMS[5][2] = {{24, 180}, {60, 198}, {290, 172}, {262, 196}, {160, 204}};
        bulbs(GEMS, 5, 19);                          /* the crystals glint */
    } else if (!strcmp(bg, "bg_nuvole")) {
        rain(0, 34, SCREEN_W, 198, 34, 20);
    } else if (!strcmp(bg, "bg_castello")) {
        twinkle_sized(132, 20, 190, 116, 4, 21, 0); /* stars in the big window */
        if (((now() / 5) & 3) != 0) {                /* the torches flicker */
            centred(GLOW_LIME, 100, 82 + (int)((now() / 7) & 1));
            centred(GLOW_LIME, 220, 82 + (int)((now() / 6) & 1));
        }
        drift(282, 164, 302, 190, 3, 22, BUBBLE, 8, true, 3, 0); /* the cauldron bubbles, from the brew up */
    } else if (!strcmp(bg, "bg_mappa") || !strcmp(bg, "bg_mappa2")) {
        twinkle_sized(0, 4, SCREEN_W, !strcmp(bg, "bg_mappa2") ? 70 : SCREEN_H, 6, 23, 0);
        if (!strcmp(bg, "bg_mappa"))
            birds(0, 230, 6, 40, 91);
        else
            shooting_star(20, 2, 250, 60, 92);
    } else if (!strcmp(bg, "bg_spiaggia")) { /* no stars (he took them): the sea glints */
        twinkle_sized(0, 128, SCREEN_W, 186, 10, 24, 0);
        twinkle_sized(60, 126, 84, 190, 3, 25, 0);
    } else if (!strcmp(bg, "bg_festa2")) {
        twinkle_sized(0, 30, SCREEN_W, 116, 16, 26, 1);
        shooting_star(40, 24, 300, 100, 93);
        twinkle_sized(0, 128, SCREEN_W, 186, 6, 27, 0);
    } else if (!strcmp(bg, "bg_giardino")) {
        fireflies(8, 96, 312, 196, 8, 28, GLOW_LIME);
        drift(0, 40, SCREEN_W, 200, 4, 29, PETAL, 7, false, 12, 30);
    } else if (!strcmp(bg, "bg_ghiaccio")) {
        drift(0, 0, SCREEN_W, 238, 22, 30, FLAKE_S, 12, false, 6, 0);
        drift(0, 0, SCREEN_W, 238, 8, 31, FLAKE, 18, false, 9, 0);
        static const int ICE[4][2] = {{34, 188}, {292, 184}, {262, 194}, {62, 198}};
        bulbs(ICE, 4, 32);
        fireflies(24, 130, 296, 200, 3, 43, GLOW_CYAN); /* little ice lights */
    } else if (!strcmp(bg, "bg_vulcano")) {
        drift(140, 0, 186, 70, 8, 33, GLOW_EMBER, 14, true, 8, 0); /* sparks from the crater */
        drift(0, 150, SCREEN_W, 236, 7, 34, GLOW_EMBER, 6, true, 5, 0);
    } else if (!strcmp(bg, "bg_torre")) {
        static const int LENS[1][2] = {{279, 120}}; /* 0.13: the telescope's lens, where the new tower has it */
        bulbs(LENS, 1, 35);
        drift(0, 60, SCREEN_W, 230, 3, 42, GLOW_GOLD, 4, true, 6, 0); /* golden motes, slowly up */
    } else if (!strcmp(bg, "bg_camerino")) {
        static const int B[18][2] = {{15, 40},  {15, 60},  {15, 80},  {15, 100}, {15, 120}, {15, 140},
                                     {15, 160}, {105, 40}, {105, 60}, {105, 80}, {105, 100}, {105, 120},
                                     {105, 140}, {105, 160}, {28, 29}, {48, 29}, {68, 29}, {88, 29}};
        bulbs(B, 18, 36);
        mirror_shine(18, 32, 103, 171, 420);
    } else if (!strcmp(bg, "bg_trucco")) {
        static const int B[20][2] = {{90, 25},  {112, 25}, {134, 25}, {156, 25}, {178, 25}, {200, 25}, {222, 25},
                                     {244, 25}, {266, 25}, {288, 25}, {77, 42},  {77, 64},  {77, 86},  {77, 108},
                                     {77, 130}, {305, 42}, {305, 64}, {305, 86}, {305, 108}, {305, 130}};
        bulbs(B, 20, 37);
    } else if (!strcmp(bg, "bg_laboratorio")) {
        twinkle_sized(28, 12, 50, 34, 2, 38, 0); /* the round window (0.13: inside its stones) */
        drift(12, 36, 66, 58, 3, 39, BUBBLE, 5, true, 2, 0);
        drift(26, 88, 54, 110, 2, 40, BUBBLE, 5, true, 2, 0); /* from the two bottles of the lower shelf */
        if (((now() / 6) % 5) != 0)                            /* the candle flickers */
            centred(GLOW_GOLD, 64, 104 + (int)((now() / 9) & 1));
    } else if (!strcmp(bg, "bg_palestra")) {
        twinkle_sized(196, 70, 290, 168, 3, 41, 0); /* dust in the sunbeam (0.13) */
    /* 0.10: the valley of music, its places, the rooms of the three new games */
    } else if (!strcmp(bg, "bg_mappa3")) {
        twinkle_sized(0, 4, SCREEN_W, 50, 5, 45, 0);
        notes(0, 60, SCREEN_W, 236, 2, 46);
        clouds(0, 8, 1, 89);
        birds(0, SCREEN_W, 8, 36, 94);
        butterflies(20, 150, 300, 230, 2, 95);
    } else if (!strcmp(bg, "bg_dolci")) {
        clouds(6, 40, 2, 88);
        twinkle_sized(0, 130, SCREEN_W, 192, 9, 47, 0); /* sugar glinting on the gumdrop hills */
        notes(0, 40, SCREEN_W, 200, 1, 48);
    } else if (!strcmp(bg, "bg_funghi")) {
        twinkle_sized(0, 4, SCREEN_W, 80, 8, 49, 1);
        fireflies(8, 70, 312, 190, 7, 50, GLOW_CYAN); /* glowing spores */
    } else if (!strcmp(bg, "bg_lago")) {
        clouds(6, 34, 2, 86);
        twinkle_sized(0, 152, SCREEN_W, 196, 10, 51, 0); /* the sun on the water */
        drift(0, 30, SCREEN_W, 200, 3, 52, PETAL, 7, false, 12, 30);
        birds(0, SCREEN_W, 10, 50, 96);
        butterflies(10, 196, 310, 232, 2, 97);
    } else if (!strcmp(bg, "bg_circo")) {
        circus_bulbs();
    } else if (!strcmp(bg, "bg_casa_orco")) {
        twinkle_sized(134, 32, 186, 76, 3, 53, 0); /* stars in the round window */
        if (((now() / 6) % 5) != 0)                /* the candle flickers */
            centred(GLOW_GOLD, 253, 52 + (int)((now() / 9) & 1));
    } else if (!strcmp(bg, "bg_biblioteca")) {
        twinkle_sized(292, 22, 316, 70, 2, 54, 0); /* dust in the lamp light */
        drift(0, 30, 78, 196, 2, 55, GLOW_GOLD, 3, true, 6, 0); /* golden motes by the books */
    } else if (!strcmp(bg, "bg_ombre")) {
        twinkle_sized(0, 0, 70, 150, 5, 56, 1);
        if (((now() / 7) % 6) != 0) { /* the two little lamps flicker */
            centred(GLOW_GOLD, 92, 159 + (int)((now() / 8) & 1));
            centred(GLOW_GOLD, 290, 159 + (int)((now() / 10) & 1));
        }
    } else if (!strcmp(bg, "bg_prato")) {
        twinkle_sized(286, 166, 320, 188, 2, 57, 0); /* the pond */
        drift(0, 0, SCREEN_W, 186, 2, 58, PETAL, 6, false, 14, 36);
    /* 0.11: the kingdom of toys (under the spell it stands still), its places, the shop, the playroom */
    } else if (!strcmp(bg, "bg_mappa4")) {
        twinkle_sized(0, 4, SCREEN_W, 40, 5, 60, 0);
        birds(0, SCREEN_W, 6, 34, 98);
        twinkle_sized(188, 158, 228, 174, 2, 61, 0); /* the pond */
        drift(0, 50, SCREEN_W, 236, 2, 62, PETAL, 6, false, 14, 36);
    } else if (!strcmp(bg, "bg_fabbrica")) {
        twinkle_sized(38, 40, 70, 72, 2, 63, 0); /* the round windows */
        twinkle_sized(250, 40, 282, 72, 2, 66, 0);
        drift(60, 40, 260, 140, 3, 67, GLOW_GOLD, 4, true, 6, 0); /* golden motes under the lamps */
    } else if (!strcmp(bg, "bg_birilli")) {
        bowling_bulbs();
        twinkle_sized(0, 0, SCREEN_W, 88, 5, 68, 1); /* the stars on the wall */
    } else if (!strcmp(bg, "bg_cubi")) {
        clouds(2, 22, 2, 87);
        twinkle_sized(0, 4, SCREEN_W, 80, 5, 69, 0);
        drift(0, 40, SCREEN_W, 200, 3, 70, PETAL, 7, false, 12, 30);
    } else if (!strcmp(bg, "bg_giostra")) {
        carousel_bulbs();
        twinkle_sized(0, 0, SCREEN_W, 80, 10, 71, 1); /* the first stars */
        if (((now() / 7) % 6) != 0) {                /* the lamp posts flicker */
            centred(GLOW_GOLD, 26, 116 + (int)((now() / 8) & 1));
            centred(GLOW_GOLD, 294, 116 + (int)((now() / 10) & 1));
        }
    } else if (!strcmp(bg, "bg_castello_giochi")) {
        twinkle_sized(136, 10, 184, 90, 3, 72, 0); /* the big window */
        drift(0, 40, SCREEN_W, 196, 3, 73, GLOW_GOLD, 3, true, 6, 0);
    } else if (!strcmp(bg, "bg_negozio")) {
        twinkle_sized(2, 20, 68, 102, 3, 74, 0); /* the toys on the shelves glint */
    } else if (!strcmp(bg, "bg_misure")) {
        twinkle_sized(288, 26, 312, 70, 2, 75, 0); /* the window */
        twinkle_sized(236, 90, 300, 190, 2, 83, 0); /* dust in the sunbeam (0.13) */
        drift(80, 20, 300, 190, 2, 76, PETAL, 6, false, 14, 36);
    /* 0.12: the rooms of the games of the old stage */
    } else if (!strcmp(bg, "bg_bacheca")) {
        clouds(0, 4, 1, 84);
        twinkle_sized(18, 14, 52, 48, 2, 77, 0); /* the sun */
        drift(0, 0, 78, 170, 2, 78, PETAL, 6, false, 14, 36);
        birds(0, SCREEN_W, 8, 24, 1);
        butterflies(4, 150, 76, 200, 2, 2);
    } else if (!strcmp(bg, "bg_pappagallo")) {
        drift(0, 0, 80, 200, 2, 79, PETAL, 6, false, 14, 36); /* petals from the tree */
        butterflies(4, 120, 70, 170, 1, 3);
    } else if (!strcmp(bg, "bg_trenino")) {
        clouds(4, 30, 2, 85);
        twinkle_sized(150, 60, 290, 92, 2, 80, 0); /* the windows of the town */
        birds(0, SCREEN_W, 22, 52, 4);
        butterflies(4, 140, 80, 175, 2, 5);
    } else if (!strcmp(bg, "bg_insegna")) {
        marquee_bulbs();
        twinkle_sized(0, 26, 62, 186, 4, 81, 1); /* the stars behind the curtains */
    } else if (!strcmp(bg, "bg_cameretta")) {
        twinkle_sized(10, 20, 64, 80, 2, 82, 0);                  /* the window */
        drift(12, 40, 120, 150, 3, 83, GLOW_GOLD, 3, true, 6, 0); /* motes in the sunlight */
    } else if (!strcmp(bg, "bg_salotto")) {
        static const int ROSE[4][2] = {{80, 28}, {302, 28}, {80, 166}, {302, 166}}; /* the gold of the frame */
        bulbs(ROSE, 4, 84);
    } else if (!strcmp(bg, "bg_libro")) {
        twinkle_sized(8, 16, 62, 76, 2, 85, 0);  /* the window */
        twinkle_sized(0, 84, 68, 146, 3, 86, 1); /* the stars of the wallpaper */
    }
}
