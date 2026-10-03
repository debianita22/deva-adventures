/* Deva's Awesome Adventures - heroine animation and rendering.
 *
 * Parts come from the atlas with their offsets on a 44x68 canvas. Since 0.13 the canvas is drawn
 * into a buffer first (at 1x, never mirrored) and the buffer goes on the screen in one piece:
 * zoomed, mirrored, squashed or stretched (gfx_blit_buffer_squash), lifted in the air. Layers,
 * back to front: hair (back), body, head, make-up, eyes, mouth, sticker, hair (front), head
 * accessory, glitter.
 *
 * The movement (0.13): she breathes, every new step of a dance lands with a little bounce, a jump
 * crouches before, stretches going up and squashes on landing, and her hair follows a moment
 * later; she walks towards us one foot after the other.
 * SPDX-License-Identifier: MIT
 */
#include "hero.h"

#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "gfx.h"

#define CANVAS_W 44
#define CANVAS_H 68
#define SPLIT 50 /* the row of the skirt where her body stretches or squashes */

enum { EYES_OPEN, EYES_CLOSED, EYES_HAPPY, EYES_SAD, EYES_ANGRY, EYES_WIDE };
enum { MOUTH_SMILE, MOUTH_OPEN, MOUTH_OH, MOUTH_FROWN, MOUTH_GRIT, MOUTH_WIDE, MOUTH_WAVY };
enum { BROW_NONE, BROW_SAD, BROW_ANGRY, BROW_UP };

typedef struct {
    int pose, eyes, mouth;
    bool flip;
    int dx, dy;
    int brow;
    int tear;   /* 0 = none, else 1 + how far the tear has run down */
    bool sweat; /* a drop of sweat on the temple (scared) */
    int lift;   /* pixels above the floor (jumps) */
    int sdy, sdx; /* the squash of the body: taller (shorter), wider (narrower), 1x pixels */
    int hair_dy, hair_dx; /* her hair, a moment late */
} frame_t;

static const struct {
    const char *sprite;
    int head_dy;
} POSES[POSE_COUNT] = {
    {"hero_body_idle", 0},   {"hero_body_idle2", 1},  {"hero_body_up", 0},
    {"hero_body_step", 0},   {"hero_body_jump", 0},   {"hero_body_giu", 4},
    {"hero_body_walk_l", 0}, {"hero_body_walk_r", 0}, {"hero_body_clap_open", 0},
    {"hero_body_clap", 0},
};
static const char *EYE_NAMES[] = {"hero_eye_open", "hero_eye_closed", "hero_eye_happy",
                                  "hero_eye_sad",  "hero_eye_angry",  "hero_eye_wide"};
static const char *EYE_R_NAMES[] = {"hero_eye_open", "hero_eye_closed",  "hero_eye_happy",
                                    "hero_eye_sad_r", "hero_eye_angry_r", "hero_eye_wide"}; /* mirrored */
static const char *MOUTH_NAMES[] = {"smile", "open", "oh", "frown", "grit", "wide", "wavy"};
static const char *BROW_NAMES[] = {NULL, "hero_brow_sad", "hero_brow_angry", "hero_brow_up"};

/* Dance: arms up, step, arms up, step the other way, a hop; every move lands on the beat. */
#define BEAT 10
#define DANCE_HOP 20 /* the hop: a crouch, up, down and the landing */
#define DANCE_LOOP (4 * BEAT + DANCE_HOP)
static const struct {
    int pose;
    bool flip;
} DANCE[4] = {{POSE_UP, false}, {POSE_STEP, false}, {POSE_UP, false}, {POSE_STEP, true}};

void hero_init(hero_t *h, int x, int feet_y)
{
    h->x = x;
    h->y = feet_y;
    h->anim = HA_IDLE;
    h->t = 0;
    h->dur = 0;
    h->blink = rng_range(90, 200);
    h->talking = false;
    h->mirror = false;
    h->move = POSE_IDLE;
    h->move_flip = false;
    h->expr = EXPR_NONE;
    h->no_shadow = false;
}

void hero_play(hero_t *h, hero_anim_t a, int dur)
{
    h->anim = a;
    h->t = 0;
    h->dur = dur;
}

void hero_move(hero_t *h, hero_pose_t pose, bool flip, int dur)
{
    h->move = pose;
    h->move_flip = flip;
    hero_play(h, HA_MOVE, dur);
}

void hero_update(hero_t *h)
{
    h->t++;
    if (h->dur > 0 && h->t >= h->dur && h->anim != HA_SLEEP)
        hero_play(h, HA_IDLE, 0);
    if (--h->blink < -7)
        h->blink = rng_range(110, 260);
}

static int talk_mouth(const hero_t *h, int rest)
{
    return h->talking ? (((h->t / 5) & 1) ? MOUTH_OPEN : MOUTH_SMILE) : rest;
}

/* a new pose lands: squashed a little for a few frames (1x pixels) */
static void land(frame_t *f, int k, int strength)
{
    if (k < 0 || k >= strength)
        return;
    f->sdy = -(strength - k);
    f->sdx = strength - k;
}

/* the body at frame t of the animation (eyes, mouth and face are added by the caller) */
static frame_t body_at(const hero_t *h, int t)
{
    frame_t f = {POSE_IDLE, EYES_OPEN, MOUTH_SMILE, false, 0, 0, BROW_NONE, 0, false, 0, 0, 0, 0, 0};
    if (t < 0)
        t = 0;
    switch (h->anim) {
    case HA_IDLE: /* she breathes: her chest a pixel up and back, slowly */
        f.sdy = swing(t, 150, 1);
        break;
    case HA_DANCE: {
        int k = t % DANCE_LOOP;
        if (k < 4 * BEAT) {
            f.pose = DANCE[k / BEAT].pose;
            f.flip = DANCE[k / BEAT].flip;
            land(&f, k % BEAT, 3);
            if (f.pose == POSE_STEP && k % BEAT < 5) /* her hair swings to the side of the step */
                f.hair_dx = f.flip ? -1 : 1;
        } else { /* the hop */
            int u = k - 4 * BEAT;
            hop_shape(u, 14, 8, 44, &f.lift, &f.sdy, &f.sdx); /* (a gentler squash than her height) */
            f.pose = u < 2 || u >= 14 ? POSE_GIU : POSE_JUMP;
        }
        break;
    }
    case HA_OH: /* whoops: a little start, taller, then a wobble */
        f.sdy = t < 10 ? (10 - t) / 4 : 0;
        f.dx = (t < 12) ? ((t / 2) & 1 ? 1 : -1) : 0;
        break;
    case HA_WAVE:
        f.pose = ((t / 14) & 1) ? POSE_STEP : POSE_UP;
        f.flip = true;
        land(&f, t % 14, 2);
        break;
    case HA_SLEEP: /* the slow, deep breaths of sleep */
        f.sdy = swing(t, 220, 2);
        break;
    case HA_MOVE: {
        f.pose = h->move;
        f.flip = h->move_flip;
        if (h->move == POSE_JUMP) { /* the jump of "Balla con me": up and down, stretched, squashed */
            int n = h->dur > 0 ? h->dur : 20;
            if (t < n) {
                hop_shape(t + n / 5, n + n / 5, 8, 44, &f.lift, &f.sdy, &f.sdx);
            }
        } else {
            land(&f, t, 3);
        }
        break;
    }
    case HA_WALK: { /* towards us: a foot up (the body a pixel higher), down (a little bounce), the other */
        int k = t % 28;
        f.pose = k < 7 ? POSE_WALK_L : (k < 14 ? POSE_IDLE : (k < 21 ? POSE_WALK_R : POSE_IDLE));
        if (f.pose != POSE_IDLE)
            f.lift = 1;
        else
            land(&f, k % 7, 2);
        f.hair_dy = f.pose != POSE_IDLE ? 1 : 0;
        break;
    }
    }
    return f;
}

static frame_t current_frame(const hero_t *h)
{
    frame_t f = body_at(h, h->t);
    /* the hair a moment late: going up it stays low, coming down it floats, landing it goes on down */
    frame_t before = body_at(h, h->t - 2);
    if (f.lift > before.lift)
        f.hair_dy += 1;
    else if (f.lift < before.lift)
        f.hair_dy -= 1;
    else if (f.sdy < 0 && f.lift == 0)
        f.hair_dy += 1;
    switch (h->anim) {
    case HA_IDLE:
        if (h->blink < 0)
            f.eyes = EYES_CLOSED;
        f.mouth = talk_mouth(h, MOUTH_SMILE);
        break;
    case HA_DANCE:
    case HA_MOVE:
        f.eyes = EYES_HAPPY;
        f.mouth = MOUTH_OPEN;
        break;
    case HA_OH:
        f.mouth = MOUTH_OH;
        break;
    case HA_WAVE:
        f.eyes = EYES_HAPPY;
        f.mouth = talk_mouth(h, MOUTH_SMILE);
        break;
    case HA_SLEEP:
        f.eyes = EYES_CLOSED;
        break;
    case HA_WALK:
        if (h->blink < 0)
            f.eyes = EYES_CLOSED;
        f.mouth = talk_mouth(h, MOUTH_SMILE);
        break;
    }
    switch (h->expr) { /* a feeling on her face */
    case EXPR_NONE:
    case EXPR_COUNT:
        break;
    case EXPR_FELICE:
        f.eyes = EYES_HAPPY;
        f.mouth = MOUTH_OPEN;
        break;
    case EXPR_TRISTE:
        f.eyes = EYES_SAD;
        f.mouth = MOUTH_FROWN;
        f.brow = BROW_SAD;
        f.tear = 1 + (h->t / 8) % 6; /* a tear running down, again and again */
        f.sdy = -swing(h->t, 160, 1); /* her shoulders down: smaller */
        break;
    case EXPR_ARRABBIATA:
        f.eyes = EYES_ANGRY;
        f.mouth = MOUTH_GRIT;
        f.brow = BROW_ANGRY;
        if (h->t % 60 < 10) { /* a little stamp of the foot, the body squashed by it */
            f.dy = (h->t % 60) < 5 ? -1 : 0;
            if (h->t % 60 >= 5)
                land(&f, h->t % 60 - 5, 3);
        }
        break;
    case EXPR_SPAVENTATA: /* worried brows, a wobbly mouth, a drop of sweat, trembling
                             (surprise: raised brows, "oh", hands up) */
        f.eyes = EYES_WIDE;
        f.mouth = MOUTH_WAVY;
        f.brow = BROW_SAD;
        f.sweat = true;
        f.dx = (h->t / 3) & 1;
        f.sdy = -1; /* hunched */
        break;
    case EXPR_SORPRESA:
        f.eyes = EYES_WIDE;
        f.mouth = MOUTH_OH;
        f.brow = BROW_UP;
        f.pose = POSE_UP; /* hands up: oh! */
        f.sdy = 2;         /* and taller */
        break;
    }
    return f;
}

/* the wand hand on the 44x68 canvas, per body pose (see tools/art/hero.py) */
static const int HAND[POSE_COUNT][2] = {{31, 46}, {30, 47}, {37, 32}, {35, 44}, {37, 32},
                                        {29, 51}, {28, 44}, {31, 48}, {28, 43}, {22, 42}};

static int s_feet[2] = {22, 63};

static void anchors(void)
{
    static bool done;
    if (!done) {
        gfx_anchor("feet", &s_feet[0], &s_feet[1]);
        done = true;
    }
}

/* where a point of the canvas lands on the screen (zoom k), after the squash and the lift */
static void canvas_to_screen(const hero_t *h, const frame_t *f, int k, int cx, int cy, int *x, int *y)
{
    bool fl = f->flip != h->mirror;
    if (fl)
        cx = CANVAS_W - cx;
    int sdx = cy >= SPLIT ? f->sdx * k : 0, sdy = f->sdy * k; /* (the top keeps its width) */
    *x = h->x + k * (cx - CANVAS_W / 2 + f->dx) + (cx < CANVAS_W / 2 ? -sdx / 2 : sdx - sdx / 2);
    *y = h->y + k * (cy - s_feet[1] + f->dy - f->lift) - (cy < SPLIT ? sdy : 0);
}

void hero_hand(const hero_t *h, int k, int *x, int *y)
{
    anchors();
    frame_t f = current_frame(h);
    k = clampi(k, 1, 4);
    canvas_to_screen(h, &f, k, HAND[f.pose][0], HAND[f.pose][1], x, y);
}

/* Draw one part at its canvas offset (or at an explicit canvas position) into the canvas buffer. */
static void part(const char *name, int dy, const int *at)
{
    const sprite_t *s = gfx_sprite(name);
    int ox = at ? at[0] : s->ox, oy = at ? at[1] : s->oy;
    gfx_blit(s, ox, oy + dy, 0);
}

static void draw_hero(const hero_t *h, const progress_t *look, int k);

void hero_draw(const hero_t *h, const progress_t *look) { draw_hero(h, look, 1); }

void hero_draw_zoom(const hero_t *h, const progress_t *look, int k) { draw_hero(h, look, clampi(k, 1, 4)); }

static uint16_t s_canvas[CANVAS_W * CANVAS_H];

static void compose(const hero_t *h, const progress_t *look, const frame_t *f)
{
    static int eye_r[2], blush_r[2], shadow_r[2];
    static bool got;
    if (!got) {
        gfx_anchor("eye_r", &eye_r[0], &eye_r[1]);
        gfx_anchor("blush_r", &blush_r[0], &blush_r[1]);
        gfx_anchor("shadow_r", &shadow_r[0], &shadow_r[1]);
        got = true;
    }
    int hd = POSES[f->pose].head_dy;
    char name[48];
    for (int i = 0; i < CANVAS_W * CANVAS_H; i++)
        s_canvas[i] = GFX_KEY;
    {
        const sprite_t *hb = gfx_sprite("hero_hair_back");
        gfx_blit(hb, hb->ox + f->hair_dx, hb->oy + hd + f->hair_dy, 0);
    }
    part(POSES[f->pose].sprite, 0, NULL);
    part("hero_head", hd, NULL);
    if (f->brow)
        part(BROW_NAMES[f->brow], hd, NULL);
    int eyes = look ? look->worn[SLOT_EYES] : -1;
    if (eyes >= 0) {
        snprintf(name, sizeof(name), "hero_shadow_%s", ITEMS[eyes].variant);
        part(name, hd, NULL);
        part(name, hd, shadow_r);
    }
    part(EYE_NAMES[f->eyes], hd, NULL);
    part(EYE_R_NAMES[f->eyes], hd, eye_r);
    if (look && look->worn[SLOT_BLUSH] >= 0) {
        part("hero_blush", hd, NULL);
        part("hero_blush", hd, blush_r);
    }
    if (f->tear)
        part("hero_tear", hd + f->tear - 1, NULL);
    int lips = look ? look->worn[SLOT_LIPS] : -1;
    if (lips >= 0)
        snprintf(name, sizeof(name), "hero_mouth_%s_%s", MOUTH_NAMES[f->mouth], ITEMS[lips].variant);
    else
        snprintf(name, sizeof(name), "hero_mouth_%s", MOUTH_NAMES[f->mouth]);
    part(name, hd, NULL);
    int sticker = look ? look->worn[SLOT_STICKER] : -1;
    if (sticker >= 0) {
        snprintf(name, sizeof(name), "hero_sticker_%s", ITEMS[sticker].variant);
        part(name, hd, NULL);
    }
    part("hero_hair_front", hd, NULL);
    if (f->sweat)
        part("hero_sweat", hd, NULL);
    int head = look ? look->worn[SLOT_HEAD] : -1;
    if (head >= 0) {
        snprintf(name, sizeof(name), "hero_%s", ITEMS[head].variant);
        part(name, hd, NULL);
    }
    if (look && look->worn[SLOT_GLITTER] >= 0) {
        /* three sparkles around the face, twinkling in turn */
        static const int spots[3][2] = {{5, 20}, {35, 24}, {7, 36}};
        const sprite_t *sp = gfx_sprite("sparkle");
        for (int i = 0; i < 3; i++)
            if (((h->t / 10) + i) % 3 != 0)
                gfx_blit(sp, spots[i][0], spots[i][1] + hd, 0);
    }
}

static void draw_hero(const hero_t *h, const progress_t *look, int k)
{
    anchors();
    frame_t f = current_frame(h);
    bool fl = f.flip != h->mirror;

    /* the ground shadow stays on the floor; high in the air it is smaller */
    const sprite_t *sh = gfx_sprite("shadow");
    if (!h->no_shadow) {
        if (f.lift > 3)
            gfx_blit_stretch(sh, h->x - sh->w * k * 3 / 8, h->y - 2 * k, sh->w * k * 3 / 4, sh->h * k, 0);
        else
            gfx_blit_scaled(sh, h->x - sh->w * k / 2, h->y - 2 * k, k, 0);
    }

    /* the canvas, at 1x, into its buffer (whatever the screen is drawing: keep its target, clip, tint) */
    uint16_t *tb, tint;
    int tw, th, cx0, cy0, cx1, cy1, tamt;
    gfx_get_target(&tb, &tw, &th);
    gfx_get_clip(&cx0, &cy0, &cx1, &cy1);
    gfx_get_tint(&tint, &tamt);
    gfx_set_tint(0, 0);
    gfx_target(s_canvas, CANVAS_W, CANVAS_H);
    compose(h, look, &f);
    gfx_target(tb, tw, th);
    gfx_set_clip(cx0, cy0, cx1, cy1);
    gfx_set_tint(tint, tamt);

    /* then on the screen: the middle of its bottom edge under her feet */
    int fx = h->x + k * f.dx, fy = h->y + k * (CANVAS_H - s_feet[1] + f.dy - f.lift);
    gfx_blit_buffer_squash(s_canvas, CANVAS_W, CANVAS_H, GFX_KEY, fx, fy, k, (fl ? GFX_FLIP_H : 0) | GFX_SQUASH_BODY,
                           SPLIT, f.sdy * k, f.sdx * k);
}
