/* Deva's Awesome Adventures - "Le misure" (0.11.0): big and small, long and
 * short, tall and low, full and empty, heavy and light.
 *
 * In the playroom of the Regno dei Giocattoli a toy friend wants something:
 * the cards show three of the same thing, different in one measure only, and
 * the question says which one ("Qual è il più alto?"). In the cards the
 * things stand on the same line (tall, big, full) or start from the same edge
 * (long), so they can be compared at a glance. Weight cannot be seen: the
 * three things hang on spring scales, and the heaviest pulls its spring down
 * the most.
 * Levels: 1 big, tall, full - very different; 2 long too, and both ways (the
 *   smallest, the emptiest...); 3 the spring scales, and closer sizes half of
 *   the times; 4 closer sizes most of the times; 5 also "in fila!": the
 *   smallest first, then the next one (the last goes by itself), on a shelf.
 * Guided help: a pink dashed line shows where the right one reaches (its top,
 *   its end, its level) on every card, or an arrow points at the spring.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

enum { D_GRANDE, D_LUNGO, D_ALTO, D_PIENO, D_PESO, D_COUNT };
#define NVAR 5    /* the sizes of each thing, smallest (or emptiest) first: mis_<kind>_<0..4> */
#define VISIBLE ((1 << D_GRANDE) | (1 << D_LUNGO) | (1 << D_ALTO) | (1 << D_PIENO))

static const char *const DIM[D_COUNT] = {"grande", "lungo", "alto", "pieno", "peso"};
/* all masculine: "Qual è il più alto?" fits each of them */
static const char *const KINDS[D_PESO][3] = {
    {"pallone", "orsetto", "regalo"},
    {"pastello", "serpente", "bruco"},
    {"albero", "girasole", "castello"},
    {"bicchiere", "barattolo", "acquario"},
};
/* weight: light, middle, heavy things (pictures pic_*) - "Quale pesa di più?" */
static const char *const WEIGHT[3][3] = {
    {"piuma", "foglia", "palloncino"},
    {"mela", "palla", "libro"},
    {"anguria", "masso", "zucca"},
};
static const char *const ASK[D_COUNT][2] = { /* [the most, the least] */
    {"mis_grande", "mis_piccolo"}, {"mis_lungo", "mis_corto"},      {"mis_alto", "mis_basso"},
    {"mis_pieno", "mis_vuoto"},    {"mis_pesa_piu", "mis_pesa_meno"},
};
static const char *const ICON[D_COUNT][2] = {
    {"mis_ico_grande", "mis_ico_piccolo"}, {"mis_ico_lungo", "mis_ico_corto"}, {"mis_ico_alto", "mis_ico_basso"},
    {"mis_ico_pieno", "mis_ico_vuoto"},    {"mis_ico_pesante", "mis_ico_leggero"},
};
static const char *const FRIENDS[] = {"robottone", "saltamolla", "dinozzo", "trottolina"};
#define NFRIENDS ARRAY_LEN(FRIENDS)

static const struct {
    int dims; /* the measures asked (bits) */
    bool both; /* "the least" too */
    int near;  /* percent of questions with close sizes */
    int row;   /* percent of "in fila" questions */
} LEVELS[5] = {
    {(1 << D_GRANDE) | (1 << D_ALTO) | (1 << D_PIENO), false, 0, 0},
    {VISIBLE, true, 0, 0},
    {VISIBLE | (1 << D_PESO), true, 50, 0},
    {VISIBLE | (1 << D_PESO), true, 75, 0},
    {VISIBLE | (1 << D_PESO), true, 60, 40},
};

/* on the cards: the line the things stand on, and the edge long things start from */
#define BASE_DY 22
#define EDGE_DX (-22)
/* the spring scales (weight) */
#define BEAM_DY (-62) /* from the centre of the panel */
#define COILS 6
static const int SPRING[3] = {8, 24, 40}; /* stretched by light, middle, heavy */
/* the shelf ("in fila") */
#define SHELF_STEP 64
#define SHELF_DY 34
/* the containers (mis_<kind>_<v>, 32x40): what is inside reaches row
   FILL_BOT - (FILL_BOT - FILL_TOP) * v / 4 (v = 4: the brim) */
#define FILL_TOP 8
#define FILL_BOT 36
/* the friend and the bubble */
#define FRIEND_DX (-58)
#define FRIEND_DY 44 /* feet */
#define GOT_LEN 20

static struct {
    int dim, least, kind;
    int var[3];   /* the size on each card (weight: 0 light, 1 middle, 2 heavy) */
    int wkind[3]; /* weight: which thing of its class */
    bool row;
    int place[3]; /* in fila: the card on each place of the shelf, -1 = empty */
    int want;     /* in fila: the place to fill */
    int friend_, last_friend, last_dim;
    int guide;    /* guided help: frames since the guide appeared, -1 = none */
    int got;      /* right answer: frames since the thing left for the friend, -1 = not yet */
    char subject[48];
} S;

/* ------------------------------------------------------------------ the things */
static const sprite_t *thing(int card)
{
    char name[32];
    if (S.dim == D_PESO)
        snprintf(name, sizeof(name), "pic_%s", WEIGHT[S.var[card]][S.wkind[card]]);
    else
        snprintf(name, sizeof(name), "mis_%s_%d", KINDS[S.dim][S.kind], S.var[card]);
    return gfx_sprite(name);
}

/* where the thing of a card is drawn, its box (x, y, w, h), at (cx, cy) */
static void thing_box(int card, int cx, int cy, int *x, int *y)
{
    const sprite_t *s = thing(card);
    switch (S.dim) {
    case D_LUNGO: /* from the same edge */
        *x = cx + EDGE_DX;
        *y = cy - s->h / 2;
        break;
    case D_PESO:
        *x = cx - s->w / 2;
        *y = cy - s->h / 2;
        break;
    default: /* standing on the same line */
        *x = cx - s->w / 2;
        *y = cy + BASE_DY - s->h;
        break;
    }
}

/* the rank of a card among the three: 0 the smallest ... 2 the biggest */
static int rank(int card)
{
    int r = 0;
    for (int i = 0; i < 3; i++)
        r += S.var[i] < S.var[card];
    return r;
}

static int card_of_rank(int r)
{
    for (int i = 0; i < 3; i++)
        if (rank(i) == r)
            return i;
    return 0;
}

/* ------------------------------------------------------------------ a new question */
static int pick_dim(int level)
{
    int dims[D_COUNT], n = 0;
    for (int d = 0; d < D_COUNT; d++)
        if (LEVELS[level - 1].dims & (1 << d))
            dims[n++] = d;
    int d;
    do
        d = dims[rng_range(0, n - 1)];
    while (n > 1 && d == S.last_dim);
    return d;
}

static void shuffle3(int *v)
{
    for (int i = 2; i > 0; i--) {
        int j = rng_range(0, i), t = v[i];
        v[i] = v[j];
        v[j] = t;
    }
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        S.last_dim = S.last_friend = -1;
    q->card = QUIZ_CARD_PICTURE;
    S.guide = -1;
    S.got = -1;
    S.row = rng_range(0, 99) < LEVELS[level - 1].row;
    S.dim = S.row ? rng_range(0, 2) : pick_dim(level); /* in fila: big, long or tall */
    S.last_dim = S.dim;
    S.least = S.row || (LEVELS[level - 1].both && rng_range(0, 1));
    S.kind = rng_range(0, 2); /* (the same measure never comes twice in a row) */
    do
        S.friend_ = rng_range(0, NFRIENDS - 1);
    while (S.friend_ == S.last_friend);
    S.last_friend = S.friend_;
    if (S.dim == D_PESO) {
        for (int i = 0; i < 3; i++) {
            S.var[i] = i;
            S.wkind[i] = rng_range(0, 2);
        }
    } else if (rng_range(0, 99) < LEVELS[level - 1].near) { /* three sizes in a row */
        int k = rng_range(0, NVAR - 3);
        if (S.dim == D_PIENO) /* "full" is the full one, "empty" the empty one */
            k = S.least ? 0 : NVAR - 3;
        for (int i = 0; i < 3; i++)
            S.var[i] = k + i;
    } else {
        for (int i = 0; i < 3; i++)
            S.var[i] = i * 2; /* very different: 0, 2, 4 */
    }
    shuffle3(S.var);
    for (int i = 0; i < 3; i++)
        S.place[i] = -1;
    S.want = 0;
    if (S.row) {
        q->steps = 2; /* the last one goes by itself */
        q->correct = card_of_rank(0);
        snprintf(S.subject, sizeof(S.subject), "fila:%s:%s", DIM[S.dim], KINDS[S.dim][S.kind]);
    } else {
        q->correct = card_of_rank(S.least ? 0 : 2);
        if (S.dim == D_PESO)
            snprintf(S.subject, sizeof(S.subject), "peso:%s:%s-%s-%s", S.least ? "meno" : "piu",
                     WEIGHT[S.var[0]][S.wkind[0]], WEIGHT[S.var[1]][S.wkind[1]], WEIGHT[S.var[2]][S.wkind[2]]);
        else
            snprintf(S.subject, sizeof(S.subject), "%s:%s:%d-%d-%d", ASK[S.dim][S.least] + 4,
                     KINDS[S.dim][S.kind], S.var[0], S.var[1], S.var[2]);
    }
}

/* ------------------------------------------------------------------ the flow */
static bool prepare(quiz_t *q)
{
    if (S.dim == D_PESO && !S.row) { /* the things drop on the scales: boing */
        for (int i = 0; i < 3; i++)
            if (q->t == 10 + i * 6)
                sfx("molla");
        return q->t > 40;
    }
    if (q->t == 4 || q->t == 12)
        sfx("pop"); /* the friend, then the bubble (or the shelf) */
    return q->t > 22;
}

static void ask(quiz_t *q)
{
    static const char *const ROW_ASK[3] = {"mis_fila_piccolo", "mis_fila_corto", "mis_fila_basso"};
    if (S.row)
        say(q->step == 0 ? ROW_ASK[S.dim] : "mis_poi");
    else
        say(ASK[S.dim][S.least]);
}

static void hover(quiz_t *q, int i)
{
    if (S.dim == D_PESO) { /* the name of the thing: it gives nothing away */
        char id[32];
        snprintf(id, sizeof(id), "w_%s", WEIGHT[S.var[i]][S.wkind[i]]);
        say(id);
    }
}

static void next_step(quiz_t *q)
{
    S.want = q->step;
    for (int p = 0; p < 3; p++)
        if (S.place[p] >= 0)
            q->off[S.place[p]] = true;
    q->correct = card_of_rank(S.want);
    S.guide = -1;
}

static void right(quiz_t *q)
{
    static const char *const THANKS[] = {"mis_grazie_1", "mis_grazie_2", "mis_grazie_3"};
    if (!S.row && !q->duel && S.dim != D_PESO) {
        S.got = 0; /* to the friend */
        say_then(THANKS[rng_range(0, ARRAY_LEN(THANKS) - 1)]);
    }
}

static bool hint(quiz_t *q)
{
    static const char *const HELP[D_COUNT] = {"mis_h_grande", "mis_h_lungo", "mis_h_alto", "mis_h_pieno",
                                              "mis_h_peso"};
    if (q->hint_step == 0) {
        say(HELP[S.dim]);
        S.guide = 0;
        return false;
    }
    say("par_scegli");
    return true;
}

static void tick(quiz_t *q)
{
    if (S.guide >= 0)
        S.guide++;
    if (S.got >= 0 && ++S.got == GOT_LEN) {
        sfx("pop");
        fx_hearts(q->panel_cx + FRIEND_DX, q->panel_cy + FRIEND_DY - 70, 3);
    }
    if (!S.row)
        return;
    if ((q->state == Q_STEP || q->state == Q_RIGHT) && S.place[S.want] < 0) {
        S.place[S.want] = q->correct; /* the chosen one goes on the shelf */
        fx_sparkles(q->panel_cx + (S.want - 1) * SHELF_STEP, q->panel_cy + SHELF_DY - 16, 12, 8);
    }
    if (q->state == Q_RIGHT && q->t == 24 && S.place[2] < 0) {
        S.place[2] = card_of_rank(2); /* and the last one by itself */
        sfx("pop");
        fx_sparkles(q->panel_cx + SHELF_STEP, q->panel_cy + SHELF_DY - 16, 12, 8);
    }
}

/* ------------------------------------------------------------------ drawing */
#define PINK rgb565(0xff, 0x4f, 0x9a)

static void dash_h(int x0, int x1, int y)
{
    for (int x = x0; x <= x1; x += 5)
        gfx_fill_rect(x, y, imin(3, x1 - x + 1), 2, PINK);
}

static void dash_v(int x, int y0, int y1)
{
    for (int y = y0; y <= y1; y += 5)
        gfx_fill_rect(x, y, 2, imin(3, y1 - y + 1), PINK);
}

/* a thin line, pixel by pixel (the strings of a scale) */
static void thin_line(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = absi(x1 - x0), dy = -absi(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (int n = 0; n < 400; n++) {
        gfx_fill_rect(x0, y0, 1, 1, c);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* the spring of a scale after the thing landed on it: it stretches with a bounce */
static int spring_len(quiz_t *q, int i)
{
    static const int8_t BOUNCE[] = {10, 7, 2, -3, -5, -4, -1, 2, 3, 2, 0, -1, -1, 0};
    int land = 10 + i * 6, full = SPRING[S.var[i]];
    if (q->state != Q_PREPARE || q->t >= land + (int)ARRAY_LEN(BOUNCE) * 2)
        return full;
    if (q->t < land)
        return 4; /* empty */
    return full + BOUNCE[(q->t - land) / 2] * (S.var[i] + 1) / 3;
}

static void draw_scales(quiz_t *q)
{
    int x0 = q->panel_cx - 104, x1 = q->panel_cx + 104, by = q->panel_cy + BEAM_DY;
    gfx_fill_rect(x0, by, x1 - x0, 5, rgb565(0xb5, 0x7a, 0x4a)); /* the wooden beam */
    gfx_fill_rect(x0, by + 4, x1 - x0, 1, rgb565(0x7a, 0x4a, 0x2a));
    gfx_fill_rect(x0, by, x1 - x0, 1, rgb565(0xe0, 0xa8, 0x70));
    const sprite_t *coil = gfx_sprite("mis_spira"), *hook = gfx_sprite("mis_gancio"), *plate = gfx_sprite("mis_piatto");
    uint16_t string = rgb565(0x6a, 0x4a, 0x5a);
    for (int i = 0; i < 3; i++) {
        int cx = quiz_card_cx(i), top = by + 5, len = spring_len(q, i);
        for (int c = 0; c < COILS; c++) /* the coils: further apart when stretched */
            gfx_blit(coil, cx - coil->w / 2, top + c * len / COILS, 0);
        int hy = top + len + coil->h - 1;
        gfx_blit(hook, cx - hook->w / 2, hy, 0);
        int py = hy + hook->h + 38; /* the plate, the thing on it, the strings */
        thin_line(cx, hy + hook->h - 1, cx - plate->w / 2 + 2, py, string);
        thin_line(cx, hy + hook->h - 1, cx + plate->w / 2 - 2, py, string);
        const sprite_t *s = thing(i);
        int land = 10 + i * 6, ty = py - s->h + 2;
        if (q->state == Q_PREPARE && q->t < land) {
            int k = land - q->t;
            ty -= k * k; /* it drops on the plate */
            if (ty + s->h < by)
                ty = -1000;
        }
        if (q->off[i])
            gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 160);
        if (ty > -1000)
            gfx_blit(s, cx - s->w / 2, ty, 0);
        gfx_blit(plate, cx - plate->w / 2, py, 0);
        gfx_set_tint(0, 0);
        if (S.guide >= 0 && i == q->correct && ((S.guide / 12) & 1) == 0) { /* this spring! */
            const sprite_t *ar = gfx_sprite(S.least ? "arrow_up" : "arrow_down");
            gfx_blit(ar, cx + 14, top + len / 2 - ar->h / 2 + ((S.guide / 6) & 1), 0);
        }
    }
}

static void draw_friend(quiz_t *q)
{
    char name[32];
    snprintf(name, sizeof(name), "mo_%s_b", FRIENDS[S.friend_]);
    const sprite_t *f = gfx_sprite(name), *b = gfx_sprite("fumetto");
    int fx_ = q->panel_cx + FRIEND_DX, fy = q->panel_cy + FRIEND_DY;
    /* 0.12: the bubble (the question) twice as big, its tail at the friend's side; in a duel only the
       bubble, in the middle */
    int bx = q->panel_cx - 30, by = q->panel_cy - 75;
    bool ready = q->state != Q_PREPARE || q->t >= 4;
    int k = 2;
    if (q->duel) {
        bx = q->panel_cx - b->w;
        by = q->panel_cy - b->h - 6;
    } else if (ready) { /* 0.13: it breathes and blinks; glad, it hops (crouch, up, a soft landing) */
        int lift = 0, sdy = 0, sdx = 0;
        if (q->state == Q_RIGHT && q->t < 90)
            hop_shape(q->t % 30, 22, 7, f->h, &lift, &sdy, &sdx);
        else if (q->state == Q_PREPARE && q->t < 12)
            hop_shape(q->t + 12, 18, 4, f->h, &lift, &sdy, &sdx); /* it lands on its place */
        actor_draw(name, fx_, fy - lift, 1, 0, 0, sdy, sdx);
    }
    if (q->state == Q_PREPARE && q->t < 12)
        return;
    gfx_blit_scaled(b, bx, by, k, 0);
    const sprite_t *ic = gfx_sprite(ICON[S.dim][S.least]);
    int bob = swing((int)G.frame, 72, 1);
    gfx_blit_scaled(ic, bx + (26 - ic->w / 2) * k, by + (18 - ic->h / 2 - bob) * k, k, 0);
    if (S.got >= 0) { /* the thing she chose flies to the friend */
        const sprite_t *s = thing(q->correct);
        int x0 = quiz_card_cx(q->correct), y0 = quiz_card_cy() - 6, x1 = fx_ + 30, y1 = fy - s->h / 2 - 2;
        int u = imin(S.got, GOT_LEN) * 256 / GOT_LEN;
        int x = x0 + (x1 - x0) * u / 256, y = y0 + (y1 - y0) * u / 256 - 44 * u * (256 - u) / 16384;
        gfx_blit(s, x - s->w / 2, y - s->h / 2, 0);
    }
}

static void draw_shelf(quiz_t *q)
{
    int y = q->panel_cy + SHELF_DY;
    const sprite_t *shelf = gfx_sprite("mis_mensola");
    gfx_blit(shelf, q->panel_cx - shelf->w / 2, y, 0);
    for (int p = 0; p < 3; p++) {
        int cx = q->panel_cx + (p - 1) * SHELF_STEP;
        gfx_number_small(p + 1, cx, y + shelf->h + 7);
        if (S.place[p] < 0) {
            if (p == S.want && q->state != Q_PREPARE && ((G.frame / 20) & 1)) {
                const sprite_t *qm = gfx_sprite("qmark");
                gfx_blit(qm, cx - qm->w / 2, y - qm->h - 4, 0);
            }
            continue;
        }
        int x, ty, dy = 0;
        if (q->state == Q_RIGHT && q->t > 24 && q->t < 110)
            dy = ((q->t / 4 + 3 - p) % 8) < 2 ? -3 : 0; /* a happy wave along the shelf */
        thing_box(S.place[p], cx, y - BASE_DY, &x, &ty);
        if (S.dim == D_LUNGO) /* long things lie on the shelf */
            ty = y - thing(S.place[p])->h;
        gfx_blit(thing(S.place[p]), x, ty + dy, 0);
    }
}

static void draw(quiz_t *q)
{
    if (S.row)
        draw_shelf(q);
    else if (S.dim == D_PESO)
        draw_scales(q);
    else
        draw_friend(q);
}

/* the guide of the guided help, on each card: where the right one reaches */
static void draw_guide(quiz_t *q, int cx, int cy)
{
    if (S.guide < 0 || S.dim == D_PESO || ((S.guide / 10) & 1))
        return;
    int x, y, c = q->correct;
    const sprite_t *s = thing(c);
    thing_box(c, cx, cy, &x, &y);
    if (S.dim == D_LUNGO) {
        dash_v(x + s->w - 1, cy - 20, cy + 20);
    } else if (S.dim == D_PIENO) { /* the level of what is inside: the brim, or the bottom */
        dash_h(cx - 24, cx + 24, y + FILL_BOT - (FILL_BOT - FILL_TOP) * S.var[c] / (NVAR - 1));
    } else {
        dash_h(cx - 24, cx + 24, y);
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (S.row) {
        for (int p = 0; p < 3; p++)
            if (S.place[p] == i)
                return; /* it is on the shelf now */
    }
    if (S.got >= 0 && i == q->correct)
        return; /* it went to the friend */
    int x, y;
    thing_box(i, cx, cy, &x, &y);
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(thing(i), x, y, 0);
    gfx_set_tint(0, 0);
    draw_guide(q, cx, cy);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    if (S.dim == D_PESO)
        snprintf(buf, n, "%s", WEIGHT[S.var[i]][S.wkind[i]]);
    else
        snprintf(buf, n, "%s%d", KINDS[S.dim][S.kind], S.var[i]);
}

static const char *subject(quiz_t *q) { return S.subject; }

const quiz_def_t QUIZ_MISURE = {
    .name = "misure",
    .game = GAME_MISURE,
    .bg = "bg_misure",
    .intro = "mis_intro",
    .card = QUIZ_CARD_PICTURE,
    .setup = setup,
    .prepare = prepare,
    .tick = tick,
    .ask = ask,
    .hover = hover,
    .right = right,
    .next_step = next_step,
    .hint = hint,
    .draw = draw,
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void) { quiz_enter(&QUIZ_MISURE); }

const scene_t SCENE_MISURE = {enter, quiz_update, quiz_draw};
