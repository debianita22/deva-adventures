/* Deva's Awesome Adventures - software renderer.
 * SPDX-License-Identifier: MIT
 */
#include "gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "stb_image.h"

uint16_t g_fb[SCREEN_W * SCREEN_H];

#define MAX_SPRITES 1024
#define HASH_SLOTS 2048
#define MAX_ANCHORS 96
#define MAX_IMAGES 64

typedef struct {
    char name[40];
    sprite_t spr;
} named_sprite_t;

static image_t s_atlas;
static named_sprite_t s_sprites[MAX_SPRITES];
static int s_nsprites;
static int16_t s_hash[HASH_SLOTS]; /* index+1 into s_sprites, 0 = empty */

static struct {
    char name[24];
    int x, y;
} s_anchors[MAX_ANCHORS];
static int s_nanchors;

static struct {
    char name[24];
    image_t img;
    bool pending; /* registered, read when first needed (or in the background) */
    bool failed;
} s_images[MAX_IMAGES];
static int s_nimages;
static char s_data_dir[512];

static const sprite_t s_empty = {0};
static uint16_t *s_fb = g_fb; /* drawing target: the screen, or a buffer (gfx_target) */
static int s_fw = SCREEN_W, s_fh = SCREEN_H;
static int s_cx0, s_cy0, s_cx1 = SCREEN_W, s_cy1 = SCREEN_H;
static uint16_t s_tint;
static int s_tint_amt;
static const sprite_t *s_big[3][10];
static const sprite_t *s_small[10];
static const sprite_t *s_font[256]; /* the small font, by Latin-1 code; NULL = no glyph */

static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

static bool load_png(const char *path, image_t *out, bool with_mask)
{
    int w, h, n;
    unsigned char *rgba = stbi_load(path, &w, &h, &n, 4);
    if (!rgba) {
        log_msg(LOG_ERROR, "cannot load %s: %s\n", path, stbi_failure_reason());
        return false;
    }
    out->w = w;
    out->h = h;
    out->px = malloc((size_t)w * h * sizeof(uint16_t));
    out->mask = with_mask ? malloc((size_t)w * h) : NULL;
    if (!out->px || (with_mask && !out->mask)) {
        stbi_image_free(rgba);
        free(out->px);
        free(out->mask);
        return false;
    }
    for (int i = 0; i < w * h; i++) {
        const unsigned char *p = rgba + i * 4;
        out->px[i] = rgb565(p[0], p[1], p[2]);
        if (with_mask)
            out->mask[i] = p[3] >= 128;
    }
    stbi_image_free(rgba);
    return true;
}

static void add_sprite(const char *name, int x, int y, int w, int h, int ox, int oy)
{
    if (s_nsprites >= MAX_SPRITES) {
        log_msg(LOG_WARN, "too many sprites: '%s' dropped (MAX_SPRITES %d)\n", name, MAX_SPRITES);
        return;
    }
    named_sprite_t *ns = &s_sprites[s_nsprites];
    snprintf(ns->name, sizeof(ns->name), "%.39s", name);
    ns->spr = (sprite_t){&s_atlas, (int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (int16_t)ox, (int16_t)oy};
    uint32_t slot = fnv1a(ns->name) & (HASH_SLOTS - 1);
    while (s_hash[slot])
        slot = (slot + 1) & (HASH_SLOTS - 1);
    s_hash[slot] = (int16_t)(s_nsprites + 1);
    s_nsprites++;
}

static bool load_atlas(const char *data_dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/gfx/atlas.png", data_dir);
    if (!load_png(path, &s_atlas, true))
        return false;
    snprintf(path, sizeof(path), "%s/gfx/atlas.txt", data_dir);
    FILE *f = fopen(path, "r");
    if (!f) {
        log_msg(LOG_ERROR, "cannot open %s\n", path);
        return false;
    }
    char line[256], name[64];
    int x, y, w, h, ox, oy;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "sprite %63s %d %d %d %d %d %d", name, &x, &y, &w, &h, &ox, &oy) == 7) {
            if (x >= 0 && y >= 0 && w > 0 && h > 0 && x + w <= s_atlas.w && y + h <= s_atlas.h)
                add_sprite(name, x, y, w, h, ox, oy);
        } else if (sscanf(line, "anchor %63s %d %d", name, &x, &y) == 3) {
            if (s_nanchors >= MAX_ANCHORS) {
                log_msg(LOG_WARN, "too many anchors: '%s' dropped (MAX_ANCHORS %d)\n", name, MAX_ANCHORS);
                continue;
            }
            snprintf(s_anchors[s_nanchors].name, sizeof(s_anchors[0].name), "%.23s", name);
            s_anchors[s_nanchors].x = x;
            s_anchors[s_nanchors].y = y;
            s_nanchors++;
        }
    }
    fclose(f);
    log_msg(LOG_INFO, "atlas %dx%d, %d sprites, %d anchors\n", s_atlas.w, s_atlas.h, s_nsprites, s_nanchors);
    return s_nsprites > 0;
}

static bool load_image(const char *data_dir, const char *name, bool mask)
{
    char path[600];
    if (s_nimages >= MAX_IMAGES)
        return false;
    snprintf(path, sizeof(path), "%s/gfx/%s.png", data_dir, name);
    if (!load_png(path, &s_images[s_nimages].img, mask))
        return false;
    snprintf(s_images[s_nimages].name, sizeof(s_images[0].name), "%s", name);
    s_nimages++;
    return true;
}

/* An image that is not needed at the start: registered now, decoded later. */
static void register_image(const char *name)
{
    if (s_nimages >= MAX_IMAGES) {
        log_msg(LOG_WARN, "too many images: '%s' dropped (MAX_IMAGES %d)\n", name, MAX_IMAGES);
        return;
    }
    memset(&s_images[s_nimages], 0, sizeof(s_images[0]));
    snprintf(s_images[s_nimages].name, sizeof(s_images[0].name), "%s", name);
    s_images[s_nimages].pending = true;
    s_nimages++;
}

static void load_pending(int i)
{
    char path[600];
    s_images[i].pending = false;
    snprintf(path, sizeof(path), "%s/gfx/%s.png", s_data_dir, s_images[i].name);
    if (!load_png(path, &s_images[i].img, false)) {
        s_images[i].failed = true;
        log_msg(LOG_WARN, "missing background %s\n", s_images[i].name);
    }
}

void gfx_prefetch_step(void)
{
    for (int i = 0; i < s_nimages; i++)
        if (s_images[i].pending) {
            load_pending(i); /* one image per call */
            return;
        }
}

bool gfx_init(const char *data_dir)
{
    static const char *bgs[] = {"bg_palco", "bg_conta", "bg_camerino", "bg_notte", "sipario"};
    gfx_free();
    if (!load_atlas(data_dir))
        return false;
    /* read when first needed: the places of the tale (the game runs without them:
       story off, black backgrounds) and the rooms of the newer games */
    static const char *tale[] = {"bg_mappa",  "bg_mappa_grigia", "bg_bosco",    "bg_palude",      "bg_grotta",
                                 "bg_nuvole", "bg_castello",     "bg_palestra", "bg_trucco",      "bg_laboratorio",
                                 "bg_mappa2", "bg_mappa2_buia",  "bg_spiaggia", "bg_giardino",    "bg_ghiaccio",
                                 "bg_vulcano", "bg_torre",       "bg_festa2",
                                 /* 0.10: the third tale, and the rooms of its three games */
                                 "bg_mappa3", "bg_mappa3_zitta", "bg_dolci",    "bg_funghi",      "bg_lago",
                                 "bg_circo",  "bg_casa_orco",    "bg_biblioteca", "bg_ombre",     "bg_prato",
                                 /* 0.11: the fourth tale, the shop and the playroom */
                                 "bg_mappa4", "bg_mappa4_ferma", "bg_fabbrica", "bg_birilli",     "bg_cubi",
                                 "bg_giostra", "bg_castello_giochi", "bg_negozio", "bg_misure",
                                 /* 0.12: a room of its own for each game of the old stage */
                                 "bg_bacheca", "bg_pappagallo", "bg_trenino", "bg_insegna", "bg_cameretta",
                                 "bg_salotto", "bg_libro"};
    snprintf(s_data_dir, sizeof(s_data_dir), "%s", data_dir);
    for (int i = 0; i < ARRAY_LEN(bgs); i++)
        if (!load_image(data_dir, bgs[i], false))
            return false;
    for (int i = 0; i < ARRAY_LEN(tale); i++)
        register_image(tale[i]);
    char name[24];
    for (int d = 0; d < 10; d++) {
        snprintf(name, sizeof(name), "dig_%d", d);
        s_big[NUM_NORMAL][d] = gfx_sprite(name);
        snprintf(name, sizeof(name), "digsel_%d", d);
        s_big[NUM_SELECTED][d] = gfx_sprite(name);
        snprintf(name, sizeof(name), "digoff_%d", d);
        s_big[NUM_OFF][d] = gfx_sprite(name);
        snprintf(name, sizeof(name), "sdig_%d", d);
        s_small[d] = gfx_sprite(name);
    }
    for (int c = 0; c < 256; c++) {
        snprintf(name, sizeof(name), "fnt_%02x", c);
        s_font[c] = gfx_has_sprite(name) ? gfx_sprite(name) : NULL;
    }
    return true;
}

void gfx_free(void)
{
    free(s_atlas.px);
    free(s_atlas.mask);
    memset(&s_atlas, 0, sizeof(s_atlas));
    for (int i = 0; i < s_nimages; i++) {
        free(s_images[i].img.px);
        free(s_images[i].img.mask);
    }
    s_nimages = s_nsprites = s_nanchors = 0;
    memset(s_hash, 0, sizeof(s_hash));
    memset(s_font, 0, sizeof(s_font));
}

const sprite_t *gfx_sprite(const char *name)
{
    uint32_t slot = fnv1a(name) & (HASH_SLOTS - 1);
    while (s_hash[slot]) {
        named_sprite_t *ns = &s_sprites[s_hash[slot] - 1];
        if (!strcmp(ns->name, name))
            return &ns->spr;
        slot = (slot + 1) & (HASH_SLOTS - 1);
    }
    static char last_missing[40];
    if (strcmp(last_missing, name)) {
        log_msg(LOG_WARN, "missing sprite '%s'\n", name);
        snprintf(last_missing, sizeof(last_missing), "%s", name);
    }
    return &s_empty;
}

bool gfx_has_sprite(const char *name)
{
    uint32_t slot = fnv1a(name) & (HASH_SLOTS - 1);
    while (s_hash[slot]) {
        if (!strcmp(s_sprites[s_hash[slot] - 1].name, name))
            return true;
        slot = (slot + 1) & (HASH_SLOTS - 1);
    }
    return false;
}

bool gfx_opaque(const sprite_t *s, int i, int j)
{
    if (!s || !s->img || i < 0 || j < 0 || i >= s->w || j >= s->h)
        return false;
    return !s->img->mask || s->img->mask[(size_t)(s->y + j) * s->img->w + s->x + i];
}

bool gfx_anchor(const char *name, int *x, int *y)
{
    for (int i = 0; i < s_nanchors; i++)
        if (!strcmp(s_anchors[i].name, name)) {
            *x = s_anchors[i].x;
            *y = s_anchors[i].y;
            return true;
        }
    return false;
}

const image_t *gfx_image(const char *name)
{
    for (int i = 0; i < s_nimages; i++)
        if (!strcmp(s_images[i].name, name)) {
            if (s_images[i].pending)
                load_pending(i); /* needed before the background got to it */
            return s_images[i].failed ? NULL : &s_images[i].img;
        }
    return NULL;
}

const image_t *gfx_image_spell(const char *name, int kind)
{
    static uint16_t px[SCREEN_W * SCREEN_H];
    static image_t img;
    static char made[40];
    static int made_kind = -1;
    const image_t *src = gfx_image(name);
    if (!src || !src->px || src->w * src->h > SCREEN_W * SCREEN_H)
        return src;
    if (made_kind == kind && !strcmp(made, name))
        return &img;
    /* the same numbers as the map filters (the speckles of dust and the mist made even) */
    static const float TINT[4][3] = {{0x9d, 0x94, 0xb8}, {0x14, 0x18, 0x3c}, {0x8c, 0x98, 0xb8}, {0x9a, 0x94, 0xb0}};
    const float *t = TINT[clampi(kind, 0, 3)];
    for (int i = 0; i < src->w * src->h; i++) {
        uint16_t p = src->px[i];
        float c[3] = {(float)(p >> 11) * (255.0f / 31.0f), (float)((p >> 5) & 63) * (255.0f / 63.0f),
                      (float)(p & 31) * (255.0f / 31.0f)};
        float lum = c[0] * 0.3f + c[1] * 0.55f + c[2] * 0.15f, o[3];
        for (int k = 0; k < 3; k++) {
            switch (kind) {
            case SPELL_DARK: o[k] = (lum * 0.35f + c[k] * 0.25f) * 0.8f + t[k] * 0.35f; break;
            case SPELL_QUIET: /* pale blue-grey, with a sleepy mist */
                o[k] = (lum * 0.6f + c[k] * 0.15f) * 0.72f + t[k] * 0.3f;
                o[k] = o[k] * 0.88f + (k == 0 ? 0xc4 : k == 1 ? 0xcc : 0xe0) * 0.12f;
                break;
            case SPELL_STILL: /* grey and dusty, a little lilac */
                o[k] = (lum * 0.62f + c[k] * 0.12f) * 0.74f + t[k] * 0.28f;
                o[k] = o[k] * 0.92f + (k == 0 ? 0xbd : k == 1 ? 0xb6 : 0xcc) * 0.08f;
                break;
            default: o[k] = (lum * 0.72f + t[k] * 0.28f) * 0.86f; break;
            }
        }
        px[i] = rgb565((uint8_t)clampi((int)o[0], 0, 255), (uint8_t)clampi((int)o[1], 0, 255),
                       (uint8_t)clampi((int)o[2], 0, 255));
    }
    img.w = src->w;
    img.h = src->h;
    img.px = px;
    img.mask = NULL;
    snprintf(made, sizeof(made), "%s", name);
    made_kind = kind;
    return &img;
}

void gfx_draw_bg(const image_t *bg)
{
    if (bg && bg->w == SCREEN_W && bg->h == SCREEN_H)
        memcpy(g_fb, bg->px, sizeof(g_fb));
    else
        memset(g_fb, 0, sizeof(g_fb));
}

void gfx_set_clip(int x0, int y0, int x1, int y1)
{
    s_cx0 = clampi(x0, 0, s_fw);
    s_cy0 = clampi(y0, 0, s_fh);
    s_cx1 = clampi(x1, s_cx0, s_fw);
    s_cy1 = clampi(y1, s_cy0, s_fh);
}

void gfx_target(uint16_t *buf, int w, int h)
{
    if (buf && w > 0 && h > 0) {
        s_fb = buf;
        s_fw = w;
        s_fh = h;
    } else {
        s_fb = g_fb;
        s_fw = SCREEN_W;
        s_fh = SCREEN_H;
    }
    gfx_set_clip(0, 0, s_fw, s_fh);
}

void gfx_get_clip(int *x0, int *y0, int *x1, int *y1)
{
    *x0 = s_cx0;
    *y0 = s_cy0;
    *x1 = s_cx1;
    *y1 = s_cy1;
}

void gfx_get_target(uint16_t **buf, int *w, int *h)
{
    *buf = s_fb;
    *w = s_fw;
    *h = s_fh;
}

void gfx_get_tint(uint16_t *color, int *amount)
{
    *color = s_tint;
    *amount = s_tint_amt;
}

void gfx_set_tint(uint16_t color, int amount)
{
    s_tint = color;
    s_tint_amt = clampi(amount, 0, 256);
}

void gfx_reset_state(void)
{
    gfx_set_clip(0, 0, s_fw, s_fh);
    s_tint_amt = 0;
}

/* A pixel mixed with a colour by t/256, channel by channel: (a * (256 - t) + b * t) >> 8.
   1.0: the pixels of a fade, a panel or a tint all mix with one colour by one amount, so the three
   channels come from small tables made once (the same arithmetic, per value instead of per pixel) */
typedef struct {
    uint16_t r[32], g[64], b[32];
} mixlut_t;

static void mixlut_make(mixlut_t *m, uint16_t color, int t)
{
    for (int v = 0; v < 32; v++) {
        m->r[v] = (uint16_t)(((v * (256 - t) + (color >> 11) * t) >> 8) << 11);
        m->b[v] = (uint16_t)((v * (256 - t) + (color & 31) * t) >> 8);
    }
    for (int v = 0; v < 64; v++)
        m->g[v] = (uint16_t)(((v * (256 - t) + ((color >> 5) & 63) * t) >> 8) << 5);
}

static inline uint16_t mixlut(const mixlut_t *m, uint16_t a)
{
    return (uint16_t)(m->r[a >> 11] | m->g[(a >> 5) & 63] | m->b[a & 31]);
}

/* the table of the current tint, remade only when the tint changes */
static const mixlut_t *tint_lut(void)
{
    static mixlut_t lut;
    static uint16_t color;
    static int amt = -1;
    if (amt != s_tint_amt || color != s_tint) {
        mixlut_make(&lut, s_tint, s_tint_amt);
        color = s_tint;
        amt = s_tint_amt;
    }
    return &lut;
}

/* A row of a sprite onto the screen where its mask says. The screen never overlaps the atlas, and
   the choice is a bitwise select: the compiler copies 8-16 pixels at a time (NEON on the console). */
static void copy_masked(uint16_t *restrict dst, const uint16_t *restrict src, const uint8_t *restrict msk, int n)
{
    for (int i = 0; i < n; i++) {
        uint16_t m = (uint16_t)-(uint16_t)(msk[i] != 0);
        dst[i] = (uint16_t)((src[i] & m) | (dst[i] & (uint16_t)~m));
    }
}

/* the same, mirrored: src and msk point at the last pixel of the row and are read backwards */
static void copy_masked_rev(uint16_t *restrict dst, const uint16_t *restrict src, const uint8_t *restrict msk, int n)
{
    for (int i = 0; i < n; i++) {
        uint16_t m = (uint16_t)-(uint16_t)(msk[-i] != 0);
        dst[i] = (uint16_t)((src[-i] & m) | (dst[i] & (uint16_t)~m));
    }
}

/* a row mixed with one colour by t/256: the mix formula on 16-bit lanes (every value fits), which
   the compiler turns into vector code */
static void fade_row(uint16_t *restrict row, int n, uint16_t color, int t)
{
    const uint16_t it = (uint16_t)(256 - t);
    const uint16_t cr = (uint16_t)((color >> 11) * t), cg = (uint16_t)(((color >> 5) & 63) * t);
    const uint16_t cb = (uint16_t)((color & 31) * t);
    for (int i = 0; i < n; i++) {
        uint16_t a = row[i];
        uint16_t r = (uint16_t)((uint16_t)((a >> 11) * it + cr) >> 8);
        uint16_t g = (uint16_t)((uint16_t)(((a >> 5) & 63) * it + cg) >> 8);
        uint16_t b = (uint16_t)((uint16_t)((a & 31) * it + cb) >> 8);
        row[i] = (uint16_t)(r << 11 | g << 5 | b);
    }
}

/* Core blitter shared by sprites and images. */
static void blit_rect(const image_t *img, int sx, int sy, int w, int h, int x, int y, int flags,
                      bool solid, uint16_t color)
{
    if (!img || !img->px)
        return;
    int i0 = x < s_cx0 ? s_cx0 - x : 0, j0 = y < s_cy0 ? s_cy0 - y : 0;
    int i1 = x + w > s_cx1 ? s_cx1 - x : w;
    int j1 = y + h > s_cy1 ? s_cy1 - y : h;
    bool flip = flags & GFX_FLIP_H;
    if (i1 <= i0)
        return;
    /* 1.0: the usual sprite (not tinted, not one colour) gets its own loops, without a test per pixel:
       where the mask says (straight or mirrored), or a plain copy */
    bool plain = !flip && !solid && !s_tint_amt, mirror = flip && !solid && !s_tint_amt;
    const mixlut_t *lut = s_tint_amt ? tint_lut() : NULL;
    for (int j = j0; j < j1; j++) {
        const uint16_t *src = img->px + (size_t)(sy + j) * img->w + sx;
        const uint8_t *msk = img->mask ? img->mask + (size_t)(sy + j) * img->w + sx : NULL;
        uint16_t *dst = s_fb + (y + j) * s_fw + x;
        if (plain && msk) {
            copy_masked(dst + i0, src + i0, msk + i0, i1 - i0);
        } else if (mirror && msk) {
            copy_masked_rev(dst + i0, src + (w - 1 - i0), msk + (w - 1 - i0), i1 - i0);
        } else if (plain) {
            memcpy(dst + i0, src + i0, (size_t)(i1 - i0) * sizeof(uint16_t));
        } else {
            for (int i = i0; i < i1; i++) {
                int si = flip ? w - 1 - i : i;
                if (msk && !msk[si])
                    continue;
                uint16_t p = solid ? color : src[si];
                dst[i] = lut ? mixlut(lut, p) : p;
            }
        }
    }
}

void gfx_blit(const sprite_t *s, int x, int y, int flags)
{
    if (s && s->img)
        blit_rect(s->img, s->x, s->y, s->w, s->h, x, y, flags, false, 0);
}

void gfx_blit_solid(const sprite_t *s, int x, int y, uint16_t color, int flags)
{
    if (s && s->img)
        blit_rect(s->img, s->x, s->y, s->w, s->h, x, y, flags, true, color);
}

void gfx_blit_scaled(const sprite_t *s, int x, int y, int k, int flags)
{
    if (!s || !s->img || !s->img->px)
        return;
    if (k <= 1) {
        gfx_blit(s, x, y, flags);
        return;
    }
    const image_t *img = s->img;
    bool flip = flags & GFX_FLIP_H;
    int j0 = clampi(s_cy0 - y, 0, s->h * k), j1 = clampi(s_cy1 - y, 0, s->h * k);
    int i0 = clampi(s_cx0 - x, 0, s->w * k), i1 = clampi(s_cx1 - x, 0, s->w * k);
    const mixlut_t *lut = s_tint_amt ? tint_lut() : NULL;
    for (int j = j0; j < j1; j++) {
        size_t row = (size_t)(s->y + j / k) * img->w + s->x;
        uint16_t *dst = s_fb + (y + j) * s_fw + x;
        int si = i0 / k, rem = i0 % k; /* (i / k, counted along the row) */
        for (int i = i0; i < i1; i++) {
            size_t idx = row + (flip ? s->w - 1 - si : si);
            if (++rem == k) {
                rem = 0;
                si++;
            }
            if (img->mask && !img->mask[idx])
                continue;
            uint16_t p = img->px[idx];
            dst[i] = lut ? mixlut(lut, p) : p;
        }
    }
}

/* ------------------------------------------------------------------ squash and stretch (0.13) */

#define MAP_MAX 1024

/* the source row (or column) of every screen row of a part split at `split` (in source pixels):
   n source pixels drawn k times bigger, d screen pixels more (repeating the split one) or fewer
   (hiding the ones just after it); returns the screen length */
static int squash_map(int16_t *map, int n, int k, int split, int d)
{
    int len = n * k + d, at = split * k;
    if (len < 1)
        len = 1;
    if (len > MAP_MAX)
        len = MAP_MAX;
    for (int j = 0; j < len; j++) {
        int src;
        if (j < at)
            src = j / k;
        else if (d >= 0)
            src = j < at + d ? split : (j - d) / k;
        else
            src = (j - d) / k; /* d < 0: skip -d screen rows after the split */
        map[j] = (int16_t)clampi(src, 0, n - 1);
    }
    return len;
}

int gfx_squash_y(int y, int k, int split_y, int dy)
{
    /* the drawing keeps its feet: everything above the split moves up by dy */
    return y < split_y ? y * k - dy : y * k;
}

/* every screen pixel of a dw x dh rectangle at (x, y) takes the pixel of its mapped row and column:
   from a sprite of the atlas (img, its corner sx0, sy0) or from a buffer sw pixels wide (key = clear) */
static void blit_mapped(const image_t *img, int sx0, int sy0, int sw, const uint16_t *buf, uint16_t key, int x, int y,
                        const int16_t *cols, int dw, const int16_t *rows, int dh, bool flip)
{
    int j0 = clampi(s_cy0 - y, 0, dh), j1 = clampi(s_cy1 - y, 0, dh);
    int i0 = clampi(s_cx0 - x, 0, dw), i1 = clampi(s_cx1 - x, 0, dw);
    static int16_t mirrored[MAP_MAX];
    if (flip) { /* mirrored: the columns read from the other end */
        for (int i = 0; i < dw; i++)
            mirrored[i] = cols[dw - 1 - i];
        cols = mirrored;
    }
    const mixlut_t *lut = s_tint_amt ? tint_lut() : NULL;
    const uint8_t *mask = img ? img->mask : NULL;
    for (int j = j0; j < j1; j++) {
        uint16_t *dst = s_fb + (y + j) * s_fw + x;
        if (img) {
            size_t row = (size_t)(sy0 + rows[j]) * img->w + sx0;
            for (int i = i0; i < i1; i++) {
                size_t idx = row + (size_t)cols[i];
                if (mask && !mask[idx])
                    continue;
                uint16_t p = img->px[idx];
                dst[i] = lut ? mixlut(lut, p) : p;
            }
        } else {
            const uint16_t *row = buf + (size_t)rows[j] * sw;
            for (int i = i0; i < i1; i++) {
                uint16_t p = row[cols[i]];
                if (p == key)
                    continue;
                dst[i] = lut ? mixlut(lut, p) : p;
            }
        }
    }
}

static void squash(const image_t *img, int sx0, int sy0, const uint16_t *buf, uint16_t key, int w, int h, int fx,
                   int fy, int k, int flags, int split_y, int dy, int dx)
{
    static int16_t rows[MAP_MAX], cols[MAP_MAX];
    k = clampi(k, 1, 8);
    split_y = clampi(split_y, 0, h - 1);
    /* never hide more than the part below the split has (the feet stay), nor half the width */
    dy = imax(dy, -(h - split_y - 1) * k);
    dx = imax(dx, -(w / 2 - 1) * k);
    int dh = squash_map(rows, h, k, split_y, dy);
    int dw = squash_map(cols, w, k, w / 2, dx); /* the columns: around the middle one */
    bool flip = flags & GFX_FLIP_H;
    if (!(flags & GFX_SQUASH_BODY) || !dx) {
        blit_mapped(img, sx0, sy0, w, buf, key, fx - dw / 2, fy - dh, cols, dw, rows, dh, flip);
        return;
    }
    /* a face above the split keeps its width: only the body below spreads or slims */
    static int16_t plain[MAP_MAX];
    int pw = squash_map(plain, w, k, w / 2, 0), js = 0;
    while (js < dh && rows[js] < split_y)
        js++;
    blit_mapped(img, sx0, sy0, w, buf, key, fx - pw / 2, fy - dh, plain, pw, rows, js, flip);
    blit_mapped(img, sx0, sy0, w, buf, key, fx - dw / 2, fy - dh + js, cols, dw, rows + js, dh - js, flip);
}

void gfx_blit_squash(const sprite_t *s, int fx, int fy, int k, int flags, int split_y, int dy, int dx)
{
    if (!s || !s->img || !s->img->px)
        return;
    if (!dy && !dx) {
        gfx_blit_scaled(s, fx - s->w * k / 2, fy - s->h * k, k, flags);
        return;
    }
    squash(s->img, s->x, s->y, NULL, 0, s->w, s->h, fx, fy, k, flags, split_y, dy, dx);
}

void gfx_blit_buffer_squash(const uint16_t *buf, int w, int h, uint16_t key, int fx, int fy, int k, int flags,
                            int split_y, int dy, int dx)
{
    if (!buf || w <= 0 || h <= 0)
        return;
    squash(NULL, 0, 0, buf, key, w, h, fx, fy, k, flags, split_y, dy, dx);
}

void gfx_blit_stretch(const sprite_t *s, int x, int y, int dw, int dh, int flags)
{
    if (!s || !s->img || !s->img->px || dw <= 0 || dh <= 0)
        return;
    if (dw == s->w && dh == s->h) {
        gfx_blit(s, x, y, flags);
        return;
    }
    static int16_t rows[MAP_MAX], cols[MAP_MAX];
    dw = imin(dw, MAP_MAX);
    dh = imin(dh, MAP_MAX);
    for (int i = 0; i < dw; i++) /* the middle of every screen pixel, back in the sprite */
        cols[i] = (int16_t)clampi((2 * i + 1) * s->w / (2 * dw), 0, s->w - 1);
    for (int j = 0; j < dh; j++)
        rows[j] = (int16_t)clampi((2 * j + 1) * s->h / (2 * dh), 0, s->h - 1);
    blit_mapped(s->img, s->x, s->y, s->w, NULL, 0, x, y, cols, dw, rows, dh, flags & GFX_FLIP_H);
}

void gfx_shine(const sprite_t *s, int x, int y, int pos, int w, int amount)
{
    if (!s || !s->img || !s->img->px || w <= 0)
        return;
    const image_t *img = s->img;
    amount = clampi(amount, 0, 256);
    mixlut_t lut;
    mixlut_make(&lut, 0xFFFF, amount);
    for (int j = 0; j < s->h; j++) {
        int yy = y + j;
        if (yy < s_cy0 || yy >= s_cy1)
            continue;
        int i0 = clampi(pos - j / 2, 0, s->w), i1 = clampi(pos - j / 2 + w, 0, s->w);
        for (int i = i0; i < i1; i++) {
            int xx = x + i;
            size_t idx = (size_t)(s->y + j) * img->w + s->x + i;
            if (xx < s_cx0 || xx >= s_cx1 || (img->mask && !img->mask[idx]))
                continue;
            uint16_t *d = &s_fb[yy * s_fw + xx];
            *d = mixlut(&lut, *d);
        }
    }
}

void gfx_blit_hsqueeze(const uint16_t *buf, int w, int h, uint16_t key, int cx, int y, int dw)
{
    if (!buf || w <= 0 || h <= 0 || dw <= 0)
        return;
    int x0 = cx - dw / 2;
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < s_cy0 || yy >= s_cy1)
            continue;
        for (int i = 0; i < dw; i++) {
            int xx = x0 + i;
            if (xx < s_cx0 || xx >= s_cx1)
                continue;
            uint16_t p = buf[j * w + i * w / dw];
            if (p != key)
                s_fb[yy * s_fw + xx] = p;
        }
    }
}

void gfx_blit_image(const image_t *img, int x, int y, int flags)
{
    if (img)
        blit_rect(img, 0, 0, img->w, img->h, x, y, flags, false, 0);
}

void gfx_fade(uint16_t color, int amount)
{
    amount = clampi(amount, 0, 256);
    if (!amount)
        return;
    for (int j = s_cy0; j < s_cy1; j++)
        fade_row(s_fb + j * s_fw + s_cx0, s_cx1 - s_cx0, color, amount);
}

void gfx_fade_outside_circle(int cx, int cy, int r, uint16_t color, int amount)
{
    amount = clampi(amount, 0, 256);
    if (!amount)
        return;
    const int soft = 10; /* the edge of the light: half as dark */
    mixlut_t full, half;
    mixlut_make(&full, color, amount);
    mixlut_make(&half, color, amount / 2);
    for (int j = s_cy0; j < s_cy1; j++) {
        uint16_t *row = s_fb + j * s_fw;
        int dy = j - cy, in = -1, out = -1;
        if (r > 0 && dy * dy <= r * r)
            in = (int)sqrtf((float)(r * r - dy * dy));
        if (dy * dy <= (r + soft) * (r + soft))
            out = (int)sqrtf((float)((r + soft) * (r + soft) - dy * dy));
        for (int i = s_cx0; i < s_cx1; i++) {
            int dx = absi(i - cx);
            if (dx <= in)
                continue;
            row[i] = mixlut(dx <= out ? &half : &full, row[i]);
        }
    }
}

/* how far a row of a rounded box is pulled in at its corners */
static int round_inset(int j, int h, int r)
{
    int d = j < r ? r - j : (j >= h - r ? j - (h - 1 - r) : 0);
    if (d <= 0)
        return 0;
    return r - (int)sqrtf((float)(r * r - d * d)) + 0;
}

void gfx_fade_round_rect(int x, int y, int w, int h, int r, uint16_t color, int amount)
{
    amount = clampi(amount, 0, 256);
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < s_cy0 || yy >= s_cy1)
            continue;
        int in = round_inset(j, h, r), x0 = imax(x + in, s_cx0), x1 = imin(x + w - in, s_cx1);
        if (x1 > x0)
            fade_row(s_fb + yy * s_fw + x0, x1 - x0, color, amount);
    }
}

static bool round_inside(int i, int j, int w, int h, int r)
{
    if (j < 0 || j >= h)
        return false;
    int in = round_inset(j, h, r);
    return i >= in && i < w - in;
}

#define ROUND_ROWS 512

/* round_inside with the insets of rows -2 .. h+1 read from a table (-1: outside the box) */
static inline bool inside_tab(const int16_t *ins, int i, int j, int w)
{
    int in = ins[j + 2];
    return in >= 0 && i >= in && i < w - in;
}

void gfx_frame_round(int x, int y, int w, int h, int r, int thick, uint16_t color, uint16_t light, uint16_t ink)
{
    /* 1.0: one square root per row instead of five per pixel, and the middle of every row below the
       top band and above the bottom one is skipped: from max(thick, r) to w - max(thick, r) the pixels
       are inside the box (no ink), not in a corner, and at least thick from every side (no band) */
    static int16_t ins[ROUND_ROWS + 4];
    bool tab = h >= 0 && h <= ROUND_ROWS;
    if (tab)
        for (int j = -2; j <= h + 1; j++)
            ins[j + 2] = (int16_t)(j < 0 || j >= h ? -1 : round_inset(j, h, r));
    int deep = imax(thick, r);
    for (int j = -1; j <= h; j++) {
        int yy = y + j;
        if (yy < s_cy0 || yy >= s_cy1)
            continue;
        uint16_t *row = s_fb + yy * s_fw;
        bool middle_row = j >= thick && j < h - thick && deep < w - deep;
        for (int i = -1; i <= w; i++) {
            if (middle_row && i == deep) {
                i = w - deep - 1; /* (the loop goes on at w - deep) */
                continue;
            }
            int xx = x + i;
            if (xx < s_cx0 || xx >= s_cx1)
                continue;
            bool inside = tab ? inside_tab(ins, i, j, w) : round_inside(i, j, w, h, r);
            if (!inside) { /* the ink line hugs the box from outside */
                if (tab ? inside_tab(ins, i - 1, j, w) || inside_tab(ins, i + 1, j, w) ||
                              inside_tab(ins, i, j - 1, w) || inside_tab(ins, i, j + 1, w)
                        : round_inside(i - 1, j, w, h, r) || round_inside(i + 1, j, w, h, r) ||
                              round_inside(i, j - 1, w, h, r) || round_inside(i, j + 1, w, h, r))
                    row[xx] = ink;
                continue;
            }
            /* how deep inside the box (along the curve in the corners), and which side is nearest */
            int dl = i, dr = w - 1 - i, dt = j, db = h - 1 - j;
            float d = (float)imin(imin(dl, dr), imin(dt, db));
            bool upleft = imin(dl, dt) <= imin(dr, db);
            if ((i < r || i >= w - r) && (j < r || j >= h - r)) {
                float ox = (float)(i < r ? r : w - 1 - r), oy = (float)(j < r ? r : h - 1 - r);
                float vx = (float)i - ox, vy = (float)j - oy;
                d = (float)r - sqrtf(vx * vx + vy * vy);
                upleft = (vy < 0 && -vy >= vx) || (vx < 0 && -vx >= vy);
            }
            if (d < (float)thick) /* the band; its inner line lighter on the upper and left sides */
                row[xx] = d >= (float)(thick - 1) && upleft ? light : color;
        }
    }
}

void gfx_shake(int dx, int dy)
{
    static uint16_t tmp[SCREEN_W * SCREEN_H];
    if (!dx && !dy)
        return;
    memcpy(tmp, g_fb, sizeof(tmp));
    for (int j = 0; j < SCREEN_H; j++) {
        int sj = j - dy;
        uint16_t *dst = g_fb + j * SCREEN_W;
        for (int i = 0; i < SCREEN_W; i++) {
            int si = i - dx;
            dst[i] = (si >= 0 && si < SCREEN_W && sj >= 0 && sj < SCREEN_H) ? tmp[sj * SCREEN_W + si] : 0;
        }
    }
}

void gfx_blit_image_circle(const image_t *img, int cx, int cy, int r)
{
    if (!img || !img->px || r <= 0)
        return;
    for (int dy = -r; dy <= r; dy++) {
        int y = cy + dy;
        if (y < s_cy0 || y >= s_cy1 || y >= img->h)
            continue;
        int half = 0;
        while ((half + 1) * (half + 1) + dy * dy <= r * r)
            half++;
        int x0 = clampi(cx - half, s_cx0, s_cx1), x1 = clampi(cx + half + 1, s_cx0, s_cx1);
        if (x1 > img->w)
            x1 = img->w;
        if (x0 < x1)
            memcpy(g_fb + y * SCREEN_W + x0, img->px + y * img->w + x0, (size_t)(x1 - x0) * 2);
    }
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    int x0 = clampi(x, s_cx0, s_cx1), x1 = clampi(x + w, s_cx0, s_cx1);
    int y0 = clampi(y, s_cy0, s_cy1), y1 = clampi(y + h, s_cy0, s_cy1);
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++)
            s_fb[j * s_fw + i] = color;
}

void gfx_blit_rotated(const uint16_t *buf, int w, int h, uint16_t key, int px, int py, int cx, int cy, int deg)
{
    /* every screen pixel around (cx, cy) takes the buffer pixel it comes from
       (inverse rotation, nearest neighbour); the key colour is transparent */
    float a = (float)deg * 3.14159265f / 180.0f, ca = cosf(a), sa = sinf(a);
    int r = 0;
    const int corners[4][2] = {{0, 0}, {w, 0}, {0, h}, {w, h}};
    for (int k = 0; k < 4; k++) {
        int dx = corners[k][0] - px, dy = corners[k][1] - py;
        int d = (int)sqrtf((float)(dx * dx + dy * dy)) + 1;
        r = d > r ? d : r;
    }
    int y0 = clampi(cy - r, s_cy0, s_cy1), y1 = clampi(cy + r + 1, s_cy0, s_cy1);
    int x0 = clampi(cx - r, s_cx0, s_cx1), x1 = clampi(cx + r + 1, s_cx0, s_cx1);
    const mixlut_t *lut = s_tint_amt ? tint_lut() : NULL;
    for (int y = y0; y < y1; y++) {
        float dy = (float)(y - cy);
        uint16_t *dst = s_fb + y * s_fw;
        for (int x = x0; x < x1; x++) {
            float dx = (float)(x - cx);
            int sx = (int)floorf((float)px + dx * ca + dy * sa + 0.5f);
            int sy = (int)floorf((float)py - dx * sa + dy * ca + 0.5f);
            if (sx < 0 || sy < 0 || sx >= w || sy >= h)
                continue;
            uint16_t p = buf[sy * w + sx];
            if (p != key)
                dst[x] = lut ? mixlut(lut, p) : p;
        }
    }
}

static int digits_of(int n, int *d)
{
    int k = 0;
    if (n <= 0) {
        d[k++] = 0;
        return k;
    }
    int tmp[6], m = 0;
    while (n > 0 && m < 6) {
        tmp[m++] = n % 10;
        n /= 10;
    }
    while (m > 0)
        d[k++] = tmp[--m];
    return k;
}

int gfx_number_big_width(int n)
{
    int d[6], k = digits_of(n, d), w = 0;
    for (int i = 0; i < k; i++)
        w += s_big[NUM_NORMAL][d[i]]->w - (i ? 1 : 0); /* outlines overlap by 1 px */
    return w;
}

void gfx_number_big(int n, int cx, int cy, int style)
{
    int d[6], k = digits_of(n, d);
    int x = cx - gfx_number_big_width(n) / 2;
    style = clampi(style, 0, 2);
    for (int i = 0; i < k; i++) {
        const sprite_t *s = s_big[style][d[i]];
        gfx_blit(s, x, cy - s->h / 2, 0);
        x += s->w - 1;
    }
}

void gfx_number_small(int n, int cx, int cy)
{
    int d[6], k = digits_of(n, d), w = 0;
    for (int i = 0; i < k; i++)
        w += s_small[d[i]]->w - (i ? 1 : 0);
    int x = cx - w / 2;
    for (int i = 0; i < k; i++) {
        const sprite_t *s = s_small[d[i]];
        gfx_blit(s, x, cy - s->h / 2, 0);
        x += s->w - 1;
    }
}

static const sprite_t *letter_sprite(char ch, int style)
{
    char name[16];
    if (ch == '?')
        return gfx_sprite("qmark");
    if (ch >= 'a' && ch <= 'z')
        ch = (char)(ch - 'a' + 'A');
    if (ch >= '0' && ch <= '9') { /* "LA STORIA 2": the digits of the big numbers, same size */
        snprintf(name, sizeof(name), "%s_%c", style == NUM_SELECTED ? "digsel" : "dig", ch);
        return gfx_sprite(name);
    }
    if (ch < 'A' || ch > 'Z')
        return NULL;
    snprintf(name, sizeof(name), "%s_%c", style == NUM_SELECTED ? "letsel" : "let", ch);
    return gfx_sprite(name);
}

#define LETTER_SPACE 8

int gfx_text_big_width(const char *s, int k)
{
    int w = 0;
    for (int i = 0; s[i]; i++) {
        const sprite_t *g = letter_sprite(s[i], NUM_NORMAL);
        w += g ? g->w + 1 : LETTER_SPACE; /* 1 px between letters */
    }
    return w > 0 ? (w - 1) * (k < 1 ? 1 : k) : 0;
}

void gfx_text_big(const char *s, int cx, int cy, int style, int k)
{
    k = k < 1 ? 1 : k;
    uint16_t old_tint = s_tint;
    int old_amt = s_tint_amt;
    if (style == NUM_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    int x = cx - gfx_text_big_width(s, k) / 2;
    for (int i = 0; s[i]; i++) {
        const sprite_t *g = letter_sprite(s[i], style);
        if (!g) {
            x += LETTER_SPACE * k;
            continue;
        }
        gfx_blit_scaled(g, x, cy - g->h * k / 2, k, 0);
        x += (g->w + 1) * k;
    }
    s_tint = old_tint;
    s_tint_amt = old_amt;
}

/* ------------------------------------------------------------------ small font (grown-up screens) */

/* Next character of a UTF-8 string as a Latin-1 code; '?' for anything beyond. */
static int next_char(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    if (!p[0])
        return 0;
    if (p[0] < 0x80) {
        *s += 1;
        return p[0];
    }
    if ((p[0] & 0xe0) == 0xc0 && (p[1] & 0xc0) == 0x80) {
        *s += 2;
        int c = ((p[0] & 0x1f) << 6) | (p[1] & 0x3f);
        return c < 256 ? c : '?';
    }
    int n = (p[0] & 0xf0) == 0xe0 ? 3 : ((p[0] & 0xf8) == 0xf0 ? 4 : 1);
    for (int i = 1; i < n; i++)
        if (!p[i]) {
            n = i;
            break;
        }
    *s += n;
    return '?';
}

static const sprite_t *font_glyph(int c)
{
    const sprite_t *g = (c >= 0 && c < 256) ? s_font[c] : NULL;
    return g ? g : s_font['?'];
}

int gfx_text_width(const char *s)
{
    int w = 0, c;
    while ((c = next_char(&s)) != 0) {
        const sprite_t *g = font_glyph(c);
        w += (g ? g->w : 3) + 1;
    }
    return w > 0 ? w - 1 : 0;
}

static int text_run(const char *s, int x, int y, uint16_t color)
{
    int c;
    while ((c = next_char(&s)) != 0) {
        const sprite_t *g = font_glyph(c);
        if (!g) {
            x += 4;
            continue;
        }
        if (c != ' ')
            gfx_blit_solid(g, x, y, color, 0);
        x += g->w + 1;
    }
    return x;
}

int gfx_text(const char *s, int x, int y, uint16_t color, int align)
{
    if (align == ALIGN_CENTER)
        x -= gfx_text_width(s) / 2;
    else if (align == ALIGN_RIGHT)
        x -= gfx_text_width(s);
    return text_run(s, x, y, color);
}

int gfx_text_shadow(const char *s, int x, int y, uint16_t color, uint16_t shadow, int align)
{
    gfx_text(s, x + 1, y + 1, shadow, align);
    return gfx_text(s, x, y, color, align);
}

void gfx_round_rect(int x, int y, int w, int h, uint16_t fill, uint16_t border)
{
    if (w < 6 || h < 6)
        return;
    /* 1 px border, corners cut by 2 px: a soft pixel-art box */
    gfx_fill_rect(x + 2, y, w - 4, 1, border);
    gfx_fill_rect(x + 2, y + h - 1, w - 4, 1, border);
    gfx_fill_rect(x, y + 2, 1, h - 4, border);
    gfx_fill_rect(x + w - 1, y + 2, 1, h - 4, border);
    gfx_fill_rect(x + 1, y + 1, 1, 1, border);
    gfx_fill_rect(x + w - 2, y + 1, 1, 1, border);
    gfx_fill_rect(x + 1, y + h - 2, 1, 1, border);
    gfx_fill_rect(x + w - 2, y + h - 2, 1, 1, border);
    gfx_fill_rect(x + 2, y + 1, w - 4, h - 2, fill);
    gfx_fill_rect(x + 1, y + 2, 1, h - 4, fill);
    gfx_fill_rect(x + w - 2, y + 2, 1, h - 4, fill);
}
