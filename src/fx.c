/* Deva's Awesome Adventures - particles.
 * SPDX-License-Identifier: MIT
 */
#include "fx.h"

#include "gfx.h"

#define MAX_P 320
#define FLY_FRAMES 26

typedef enum { P_CONFETTI, P_SPARKLE, P_ZED, P_TWINKLE, P_BURST, P_HEART, P_PUFF, P_FALL, P_FLY } ptype_t;

typedef struct {
    ptype_t type;
    int32_t x, y, vx, vy; /* 24.8 fixed point */
    int32_t x1, y1;       /* P_FLY: where it lands */
    int life, age;
    uint16_t color;       /* confetti colour; the tint of twinkles, bursts, falling stars */
    uint8_t size;         /* hearts: 0 small, 1 medium */
    bool active;
} particle_t;

static particle_t s_p[MAX_P];
static int s_flying;               /* P_FLY stars in the air */
static uint32_t s_landed_at;       /* frame counter of the last landing */
static uint32_t s_frames = 100000; /* our own clock: fx_update calls */

static const uint8_t CONFETTI_RGB[][3] = {
    {0xff, 0x93, 0xc6}, {0xff, 0xd2, 0x3f}, {0x4f, 0xd6, 0xc0},
    {0x74, 0xb8, 0xff}, {0xb3, 0x76, 0xec}, {0xff, 0xff, 0xff},
};

/* sin(i * 2pi / 64) * 64 */
static const int8_t SIN64[64] = {0,   6,   12,  19,  24,  30,  36,  41,  45,  49,  53,  56,  59,  61,  63,  64,
                                 64,  64,  63,  61,  59,  56,  53,  49,  45,  41,  36,  30,  24,  19,  12,  6,
                                 0,   -6,  -12, -19, -24, -30, -36, -41, -45, -49, -53, -56, -59, -61, -63, -64,
                                 -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36, -30, -24, -19, -12, -6};

static uint16_t rainbow(int i)
{
    const uint8_t *c = CONFETTI_RGB[((i % 5) + 5) % 5]; /* no white: a colour you can see */
    return rgb565(c[0], c[1], c[2]);
}

static particle_t *spawn(void)
{
    for (int i = 0; i < MAX_P; i++)
        if (!s_p[i].active) {
            s_p[i] = (particle_t){0};
            s_p[i].active = true;
            return &s_p[i];
        }
    return NULL;
}

void fx_clear(void)
{
    for (int i = 0; i < MAX_P; i++)
        s_p[i].active = false;
    s_flying = 0;
}

void fx_confetti(int x, int y, int n)
{
    for (int i = 0; i < n; i++) {
        particle_t *p = spawn();
        if (!p)
            return;
        const uint8_t *c = CONFETTI_RGB[rng_range(0, ARRAY_LEN(CONFETTI_RGB) - 1)];
        p->type = P_CONFETTI;
        p->x = x * 256;
        p->y = y * 256;
        p->vx = rng_range(-420, 420);
        p->vy = rng_range(-900, -350);
        p->life = rng_range(45, 80);
        p->color = rgb565(c[0], c[1], c[2]);
    }
}

void fx_sparkles(int x, int y, int radius, int n)
{
    for (int i = 0; i < n; i++) {
        particle_t *p = spawn();
        if (!p)
            return;
        p->type = P_SPARKLE;
        p->x = (x + rng_range(-radius, radius)) * 256; /* fixed point, x may be < 0 */
        p->y = (y + rng_range(-radius, radius)) * 256;
        p->vx = 0;
        p->vy = -40;
        p->life = rng_range(20, 40);
        p->age = -rng_range(0, 20); /* staggered start */
    }
}

void fx_zed(int x, int y)
{
    particle_t *p = spawn();
    if (!p)
        return;
    p->type = P_ZED;
    p->x = x * 256;
    p->y = y * 256;
    p->vx = 60;
    p->vy = -90;
    p->life = 110;
}

void fx_twinkle(int x, int y, int delay)
{
    particle_t *p = spawn();
    if (!p)
        return;
    p->type = P_TWINKLE;
    p->x = x * 256;
    p->y = y * 256;
    p->vy = -20;
    p->life = 14;
    p->age = -delay;
    p->color = rainbow(rng_range(0, 4));
}

void fx_trail(int x0, int y0, int x1, int y1, int n)
{
    for (int i = 0; i < n; i++) {
        int x = x0 + (x1 - x0) * (i + 1) / (n + 1), y = y0 + (y1 - y0) * (i + 1) / (n + 1);
        int dx, dy;
        rng_pair(-2, 2, -3, 3, &dx, &dy);
        fx_twinkle(x + dx, y + dy, i * 2);
    }
}

void fx_burst(int x, int y, int n)
{
    int phase = rng_range(0, 63);
    for (int i = 0; i < n; i++) {
        particle_t *p = spawn();
        if (!p)
            return;
        int a = (phase + i * 64 / n) & 63;
        p->type = P_BURST;
        p->x = x * 256;
        p->y = y * 256;
        p->vx = SIN64[(a + 16) & 63] * 9; /* ~2.2 px per frame, slowing down */
        p->vy = SIN64[a] * 9;
        p->life = 30;
        p->color = rainbow(i);
    }
}

void fx_hearts(int x, int y, int n)
{
    for (int i = 0; i < n; i++) {
        particle_t *p = spawn();
        if (!p)
            return;
        p->type = P_HEART;
        p->x = (x + rng_range(-10, 10)) * 256;
        p->y = y * 256;
        p->vy = -rng_range(120, 190);
        p->vx = rng_range(0, 63); /* the phase of its sway */
        p->life = rng_range(55, 75);
        p->age = -i * 9;
        p->size = (uint8_t)(i == 0 || rng_range(0, 2) == 0);
    }
}

void fx_puff(int x, int y)
{
    particle_t *p = spawn();
    if (!p)
        return;
    p->type = P_PUFF;
    p->x = x * 256;
    p->y = y * 256;
    p->vy = -40;
    p->life = 40;
}

void fx_starfall(int n)
{
    for (int i = 0; i < n; i++) {
        particle_t *p = spawn();
        if (!p)
            return;
        p->type = P_FALL;
        p->x = rng_range(8, SCREEN_W - 8) * 256;
        p->y = -rng_range(4, 40) * 256;
        p->vy = rng_range(220, 380);
        p->vx = rng_range(0, 63);
        p->life = rng_range(70, 110);
        p->age = -rng_range(0, 40);
        p->color = rainbow(i);
    }
}

void fx_fly_star(int x0, int y0, int x1, int y1)
{
    particle_t *p = spawn();
    if (!p)
        return;
    p->type = P_FLY;
    p->x = x0 * 256;
    p->y = y0 * 256;
    p->vx = x0 * 256; /* the start, kept for the arc */
    p->vy = y0 * 256;
    p->x1 = x1 * 256;
    p->y1 = y1 * 256;
    p->life = FLY_FRAMES;
    s_flying++;
}

int fx_stars_flying(void) { return s_flying; }

int fx_star_landed_age(void) { return (int)(s_frames - s_landed_at); }

static void land(particle_t *p)
{
    s_flying = s_flying > 0 ? s_flying - 1 : 0;
    s_landed_at = s_frames;
    int x = p->x1 / 256, y = p->y1 / 256;
    fx_sparkles(x, y, 7, 6);
    for (int i = 0; i < 4; i++)
        fx_twinkle(x + (i & 1 ? 7 : -7), y + (i & 2 ? 6 : -6), i * 2);
}

void fx_update(void)
{
    s_frames++;
    for (int i = 0; i < MAX_P; i++) {
        particle_t *p = &s_p[i];
        if (!p->active)
            continue;
        if (++p->age >= p->life) {
            p->active = false;
            if (p->type == P_FLY)
                land(p);
            continue;
        }
        if (p->age < 0)
            continue;
        switch (p->type) {
        case P_CONFETTI:
            p->x += p->vx;
            p->y += p->vy;
            p->vy += 38;                 /* gravity */
            p->vx = p->vx * 31 / 32;      /* air drag */
            if (p->vy > 380)
                p->vy = 380;
            break;
        case P_ZED:
            p->x += p->vx;
            p->y += p->vy;
            p->vx = ((p->age / 20) & 1) ? 60 : -20; /* lazy zig-zag */
            break;
        case P_BURST:
            p->x += p->vx;
            p->y += p->vy;
            p->vx = p->vx * 7 / 8;
            p->vy = p->vy * 7 / 8 + 6;
            break;
        case P_HEART: /* up, swaying */
            p->y += p->vy;
            p->x += SIN64[(p->vx + p->age * 2) & 63] * 3 / 2;
            break;
        case P_FALL:
            p->y += p->vy;
            p->x += SIN64[(p->vx + p->age * 3) & 63] * 2;
            break;
        case P_FLY: { /* along an arc: ease out, 30 px over the straight line */
            int t = p->age * 256 / FLY_FRAMES;
            int e = 256 - (256 - t) * (256 - t) / 256;
            p->x = p->vx + (int32_t)((int64_t)(p->x1 - p->vx) * e / 256);
            p->y = p->vy + (int32_t)((int64_t)(p->y1 - p->vy) * e / 256) - (e * (256 - e) / 256) * 30 * 256 / 64;
            if ((p->age & 1) == 0) {
                int dx, dy;
                rng_pair(-2, 2, -2, 2, &dx, &dy);
                fx_twinkle(p->x / 256 + dx, p->y / 256 + dy, 0);
            }
            break;
        }
        default:
            p->x += p->vx;
            p->y += p->vy;
            break;
        }
    }
}

static const sprite_t *s_tw[4], *s_spark, *s_zs, *s_zl, *s_heart[2], *s_puff[3], *s_star;

static void sprites(void)
{
    static bool done;
    if (done)
        return;
    done = true;
    static const char *const TWN[4] = {"tw_0", "tw_1", "tw_2", "tw_3"};
    static const char *const PUFFN[3] = {"puff_0", "puff_1", "puff_2"};
    for (int i = 0; i < 4; i++)
        s_tw[i] = gfx_sprite(TWN[i]);
    for (int i = 0; i < 3; i++)
        s_puff[i] = gfx_sprite(PUFFN[i]);
    s_spark = gfx_sprite("sparkle");
    s_zs = gfx_sprite("zed_s");
    s_zl = gfx_sprite("zed_l");
    s_heart[0] = gfx_sprite("heart_s");
    s_heart[1] = gfx_sprite("heart_m");
    s_star = gfx_sprite("star_on");
}

static void centred(const sprite_t *s, int x, int y) { gfx_blit(s, x - s->w / 2, y - s->h / 2, 0); }

void fx_draw(void)
{
    sprites();
    static const uint8_t TW[14] = {0, 1, 1, 2, 2, 3, 3, 3, 2, 2, 1, 1, 0, 0}; /* grows, shines, fades */
    for (int i = 0; i < MAX_P; i++) {
        const particle_t *p = &s_p[i];
        if (!p->active || p->age < 0)
            continue;
        int x = p->x / 256, y = p->y / 256;
        switch (p->type) {
        case P_CONFETTI: {
            int w = (p->age / 4) & 1 ? 3 : 2; /* flutter */
            gfx_fill_rect(x, y, w, 5 - w, p->color);
            break;
        }
        case P_SPARKLE:
            if ((p->age / 4) % 3 != 2)
                centred(s_spark, x, y);
            break;
        case P_ZED:
            gfx_blit(p->age < 50 ? s_zs : s_zl, x, y, 0);
            break;
        case P_TWINKLE:
            gfx_set_tint(p->color, 90);
            centred(s_tw[TW[clampi(p->age, 0, 13)]], x, y);
            gfx_set_tint(0, 0);
            break;
        case P_BURST: {
            int k = p->age < 10 ? 3 : (p->age < 18 ? 2 : (p->age < 25 ? 1 : 0));
            gfx_set_tint(p->color, 110);
            centred(s_tw[k], x, y);
            gfx_set_tint(0, 0);
            break;
        }
        case P_HEART:
            if (p->life - p->age > 10 || ((p->age / 3) & 1))
                centred(s_heart[p->size], x, y);
            break;
        case P_PUFF:
            centred(s_puff[p->age < 5 ? 0 : (p->age < 32 ? 1 : 2)], x, y); /* pops, sleeps, comes apart */
            break;
        case P_FALL: {
            int k = ((p->age / 6) & 1) ? 2 : 1;
            gfx_set_tint(p->color, 110);
            centred(s_tw[k], x, y);
            gfx_set_tint(0, 0);
            break;
        }
        case P_FLY:
            centred(s_star, x, y);
            break;
        }
    }
}
