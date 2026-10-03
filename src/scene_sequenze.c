/* Deva's Awesome Adventures - "Sequenze": a row of pictures follows a rhythm
 * (AB, AAB, ABB, ABC, AABB); she picks what comes next or, from level 4,
 * what is missing in the middle. The guided help names every picture in
 * rhythm ("stella, cuore, stella, cuore...") so the pattern can be heard.
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

#define MAX_SLOTS 7
#define SLOT 30
#define SLOT_STEP 32
#define POP_EVERY 12 /* frames between two pictures appearing: the rhythm */

/* pictures grouped by main colour: the three of a row come from different
 * groups, so the pattern is carried by both shape and colour */
static const struct {
    const char *w;
    char group;
} PICS[] = {
    {"stella", 'Y'}, {"sole", 'Y'},     {"luna", 'Y'},    {"banana", 'Y'}, {"cuore", 'R'},
    {"mela", 'R'},   {"palloncino", 'R'}, {"diamante", 'B'}, {"fiocco", 'B'}, {"farfalla", 'T'},
    {"uva", 'V'},    {"smalto", 'V'},   {"pesce", 'O'},   {"fiore", 'W'},  {"uovo", 'W'},
    {"elefante", 'G'}, {"gelato", 'P'}, {"torta", 'P'},
};
#define NPICS ARRAY_LEN(PICS)

typedef struct {
    const char *name; /* for the log */
    const char *unit; /* one period, letters = distinct pictures */
    int len;          /* pictures in the row, the gap included */
} pattern_t;

static const pattern_t PATTERNS[] = {
    {"AB", "AB", 6}, {"AAB", "AAB", 6}, {"ABB", "ABB", 6}, {"ABC", "ABC", 6}, {"AABB", "AABB", 7},
};
enum { P_AB, P_AAB, P_ABB, P_ABC, P_AABB };

static struct {
    int pat, len, gap;       /* gap = index of the "?" slot */
    int row[MAX_SLOTS];      /* picture index per slot */
    int opt[3];              /* picture per card */
    bool missing;            /* "che cosa manca?" instead of "che cosa viene dopo?" */
    int last_first;
    int hint_i;              /* slot being named by the guided help, -1 = none */
} S;

static void choose_pattern(int level)
{
    static const int L1[] = {P_AB}, L2[] = {P_AB, P_AAB, P_ABB}, L3[] = {P_ABC, P_AABB, P_AAB},
                     L4[] = {P_AB, P_AAB, P_ABC}, L5[] = {P_ABC, P_AABB, P_ABB, P_AAB};
    static const int *SETS[5] = {L1, L2, L3, L4, L5};
    static const int NSET[5] = {ARRAY_LEN(L1), ARRAY_LEN(L2), ARRAY_LEN(L3), ARRAY_LEN(L4), ARRAY_LEN(L5)};
    int l = clampi(level, 1, 5) - 1;
    S.pat = SETS[l][rng_range(0, NSET[l] - 1)];
    S.missing = level == 4 || (level == 5 && rng_range(0, 1));
}

static bool group_used(const int *pics, int n, char g)
{
    for (int i = 0; i < n; i++)
        if (PICS[pics[i]].group == g)
            return true;
    return false;
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        S.last_first = -1;
    choose_pattern(level);
    const pattern_t *p = &PATTERNS[S.pat];
    S.len = p->len;

    /* distinct pictures A, B, C (+ one outsider for the cards) */
    int pics[4], n = 0;
    while (n < 4) {
        int k = rng_range(0, NPICS - 1);
        if (group_used(pics, n, PICS[k].group) || (n == 0 && k == S.last_first))
            continue;
        pics[n++] = k;
    }
    S.last_first = pics[0];
    int period = (int)strlen(p->unit);
    for (int i = 0; i < S.len; i++)
        S.row[i] = pics[p->unit[i % period] - 'A'];

    /* "?" at the end, or somewhere after the first full period */
    S.gap = S.missing ? rng_range(period, S.len - 2) : S.len - 1;
    int answer = S.row[S.gap];

    /* cards: the answer, another picture of the row, the outsider */
    int other = -1;
    for (int i = 0; i < S.len && other < 0; i++)
        if (S.row[i] != answer)
            other = S.row[i];
    int cards[3] = {answer, other, pics[3]};
    for (int i = 2; i > 0; i--) {
        int j = rng_range(0, i), t = cards[i];
        cards[i] = cards[j];
        cards[j] = t;
    }
    for (int i = 0; i < 3; i++) {
        S.opt[i] = cards[i];
        if (cards[i] == answer)
            q->correct = i;
    }
    S.hint_i = -1;
}

static int slot_x(quiz_t *q, int i) { return q->panel_cx - (S.len * SLOT_STEP - 2) / 2 + i * SLOT_STEP; }
static int slot_y(quiz_t *q) { return q->panel_cy - SLOT / 2 - 4; }

/* 0.12: under the sky of bg_trenino the row rides a toy train: the engine in front (on the left,
   it pulls), a wagon for each picture, the big pictures sitting in them, the gap a golden wagon.
   The duels keep the row of little cards. */
#define RAIL_Y 132 /* the top of the rails (tools/art/games6.py) */
#define LOCO_W 46
#define WAGON_W 36
#define WAGON_STEP 38
#define WAGON_Y (RAIL_Y - 25)

static int wagon_x(int i) { return (SCREEN_W - (LOCO_W + S.len * WAGON_STEP)) / 2 + LOCO_W + i * WAGON_STEP; }

/* the centre of place i of the row, wherever it is drawn */
static void place_centre(quiz_t *q, int i, int *x, int *y)
{
    if (q->duel) {
        *x = slot_x(q, i) + SLOT / 2;
        *y = slot_y(q) + SLOT / 2;
    } else {
        *x = wagon_x(i) + WAGON_W / 2;
        *y = WAGON_Y - 8;
    }
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1 && !q->duel)
        sfx("treno");
    if (!q->duel && q->t % 30 == 2) /* steam from the chimney while the wagons are loaded */
        fx_puff(wagon_x(0) - LOCO_W + 15, RAIL_Y - 40);
    /* one picture every POP_EVERY frames, Deva stepping on the beat */
    if (q->t % POP_EVERY == 1 && q->t / POP_EVERY < S.len) {
        int i = q->t / POP_EVERY;
        if (i == S.gap)
            sfx("blip");
        else
            sfx("pop");
        hero_move(&G.hero, (i & 1) ? POSE_UP : POSE_STEP, i & 2, POP_EVERY);
    }
    return q->t >= S.len * POP_EVERY + 16;
}

static void say_pic(int k, bool queue)
{
    char id[40];
    snprintf(id, sizeof(id), "w_%s", PICS[k].w);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void ask(quiz_t *q) { say(S.missing ? "seq_manca" : "seq_dopo"); }

static void hover(quiz_t *q, int i) { say_pic(S.opt[i], false); }

static void right(quiz_t *q)
{
    int x, y;
    say_pic(S.opt[q->correct], true);
    place_centre(q, S.gap, &x, &y);
    fx_sparkles(x, y, 12, 10);
    if (!q->duel) { /* the train is full: toot toot */
        sfx("treno");
        fx_puff(wagon_x(0) - LOCO_W + 15, RAIL_Y - 40);
    }
}

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("seq_intro"); /* "Guarda bene la fila!" */
        return false;
    }
    int i = q->hint_step - 1;
    if (i < S.len) {
        S.hint_i = i;
        hero_move(&G.hero, (i & 1) ? POSE_UP : POSE_STEP, i & 2, 20);
        if (i == S.gap) {
            sfx("blip"); /* the gap: a beat of silence */
            q->hint_wait = 40;
        } else
            say_pic(S.row[i], false);
        return false;
    }
    S.hint_i = -1;
    say("par_scegli");
    return true;
}

static void draw_pic(int k, int cx, int cy, int zoom)
{
    char name[40];
    snprintf(name, sizeof(name), "%s_%s", zoom ? "pic" : "pics", PICS[k].w);
    const sprite_t *s = gfx_sprite(name);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
}

static void draw_train(quiz_t *q, int shown)
{
    static const char *WAGONS[4] = {"vagone_0", "vagone_1", "vagone_2", "vagone_3"};
    bool solved = q->state == Q_RIGHT;
    /* the engine chugs: a little jolt now and then while it is loaded, and when the row is right */
    int jolt = (q->state == Q_PREPARE || (solved && q->t < 90)) && ((q->t / 6) % 3 == 0) ? -1 : 0;
    gfx_blit(gfx_sprite("loco"), wagon_x(0) - LOCO_W, RAIL_Y - 40 + jolt, 0);
    for (int i = 0; i < S.len; i++) {
        int x = wagon_x(i), dy = 0;
        if (q->state == Q_PREPARE && i == shown - 1 && q->t % POP_EVERY < 5)
            dy = -3; /* just popped in */
        if (solved && q->t < 90)
            dy = ((q->t / 4 + S.len - i) % 8) < 2 ? -3 : 0; /* happy wave */
        if (i == S.hint_i && voice_busy())
            dy = -3;
        /* the picture first: the side of the wagon hides its bottom */
        if (i < shown) {
            if (i == S.gap && !solved) {
                const sprite_t *qm = gfx_sprite("qmark");
                int bob = swing((int)G.frame, 60, 1);
                gfx_blit_scaled(qm, x + WAGON_W / 2 - qm->w, WAGON_Y + 10 - 2 * qm->h + dy - bob, 2, 0);
            } else {
                draw_pic(S.row[i], x + WAGON_W / 2, WAGON_Y - 8 + dy, 1);
            }
        }
        gfx_blit(gfx_sprite(i == S.gap ? "vagone_sel" : WAGONS[i % 4]), x, WAGON_Y, 0);
    }
}

static void draw(quiz_t *q)
{
    int shown = q->state == Q_PREPARE ? q->t / POP_EVERY + (q->t % POP_EVERY ? 1 : 0) : S.len;
    bool solved = q->state == Q_RIGHT;
    if (!q->duel) {
        draw_train(q, shown);
        return;
    }
    for (int i = 0; i < S.len && i < shown; i++) {
        int x = slot_x(q, i), y = slot_y(q);
        int dy = 0;
        if (q->state == Q_PREPARE && i == shown - 1 && q->t % POP_EVERY < 5)
            dy = -2; /* just popped */
        if (solved && q->t < 90)
            dy = ((q->t / 4 + S.len - i) % 8) < 2 ? -3 : 0; /* happy wave */
        if (i == S.hint_i && voice_busy())
            dy = -3;
        if (i == S.gap) {
            gfx_blit(gfx_sprite("scard_sel"), x - 2, y - 2 + dy, 0);
            if (solved) {
                draw_pic(S.row[i], x + SLOT / 2, y + SLOT / 2 + dy, 0);
            } else {
                const sprite_t *qm = gfx_sprite("qmark");
                int bob = swing((int)G.frame, 60, 1);
                gfx_blit(qm, x + (SLOT - qm->w) / 2, y + (SLOT - qm->h) / 2 + dy - bob, 0);
            }
        } else {
            gfx_blit(gfx_sprite("scard"), x, y + dy, 0);
            draw_pic(S.row[i], x + SLOT / 2, y + SLOT / 2 + dy, 0);
        }
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    draw_pic(S.opt[i], cx, cy, 1);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", PICS[S.opt[i]].w); }

/* e.g. "dopo-AB:ABABA?" or "manca-ABC:ABC?BC" */
static const char *subject(quiz_t *q)
{
    static char s[48];
    int seen[MAX_SLOTS], nseen = 0;
    int pos = snprintf(s, sizeof(s), "%s-%s:", S.missing ? "manca" : "dopo", PATTERNS[S.pat].name);
    for (int i = 0; i < S.len && pos < (int)sizeof(s) - 1; i++) {
        int k = 0;
        while (k < nseen && seen[k] != S.row[i])
            k++;
        if (k == nseen)
            seen[nseen++] = S.row[i];
        s[pos++] = i == S.gap ? '?' : (char)('A' + k);
    }
    s[pos] = 0;
    return s;
}

const quiz_def_t QUIZ_SEQUENZE = {
    .name = "sequenze",
    .game = GAME_SEQUENZE,
    .bg = "bg_trenino", /* 0.12: a toy railway, the row rides the wagons */
    .intro = "seq_intro",
    .card = QUIZ_CARD_PICTURE,
    .setup = setup,
    .prepare = prepare,
    .ask = ask,
    .hover = hover,
    .right = right,
    .hint = hint,
    .draw = draw,
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void) { quiz_enter(&QUIZ_SEQUENZE); }

const scene_t SCENE_SEQUENZE = {enter, quiz_update, quiz_draw};
