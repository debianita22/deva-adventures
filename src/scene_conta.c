/* Deva's Awesome Adventures - "Conta": numbers and quantities.
 *
 * Five kinds of question, mixed by level (the flow is the shared quiz engine):
 *   conta    objects drop in, "Quante stelline ci sono?"          -> the number
 *   piu      three groups, "Dove ce ne sono di più?"              -> the card under it
 *   dopo     number line 5 6 7 ?, "Che numero viene dopo?"        -> 8
 *   prima    number line ? 8 9 10, "Che numero viene prima?"      -> 7
 *   insieme  3 stars + 2 stars, "Quante sono in tutto?"           -> 5
 * The guided help counts the objects aloud (numbering them), reads the
 * number line, or shows how many are in each group.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>

#include "anim.h"
#include "audio.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

#define NOBJ 10
#define MAXN 24
#define OBJ_SMALL 24 /* dense grids, groups */
#define OBJ_BIG 36   /* a handful, scattered: easier for small eyes */
#define OBJ_HUGE 48  /* up to six: big, on the spots of a die (0.12: seen at a glance, then counted) */
#define BOX_W 44     /* number line boxes */
#define BOX_H 36
#define BOX_STEP 50

static const char *OBJ_NAMES[NOBJ] = {"stelle",    "cuori",     "rossetti", "smalti",  "fiocchi",
                                      "microfoni", "scarpette", "farfalle", "fiori",   "diamanti"};

enum { M_CONTA, M_PIU, M_DOPO, M_PRIMA, M_INSIEME, M_COUNT };

static const struct {
    int lo, hi;
    bool grid;
    int mix[M_COUNT]; /* percent of each kind of question */
} LEVELS[5] = {
    {1, 5, false, {100, 0, 0, 0, 0}},
    {3, 8, false, {70, 30, 0, 0, 0}},
    {6, 12, true, {50, 20, 30, 0, 0}},
    {10, 16, true, {40, 15, 15, 0, 30}},
    {12, 20, true, {40, 15, 10, 15, 20}},
};

static struct {
    int mode, level;
    int obj, last_obj, count, last_count;
    int opt[3];
    int n;                  /* objects on stage */
    int px[MAXN], py[MAXN], grp[MAXN];
    int osz;                /* object size for this question */
    int hint_i;             /* objects counted aloud so far - 1 */
    int group_n[3];         /* piu: objects per group; insieme: a, b */
    bool show_counts;       /* piu: numbers on the cards */
    int line0;              /* dopo/prima: first number of the line */
    bool line_filled;       /* the "?" box shows the answer */
    char subject[32];
} S;

static void say_number(int n, bool queue)
{
    char id[16];
    snprintf(id, sizeof(id), "n%02d", clampi(n, 1, 20));
    if (queue)
        say_then(id);
    else
        say(id);
}

static int pick_mode(int level)
{
    int r = rng_range(0, 99), acc = 0;
    for (int m = 0; m < M_COUNT; m++) {
        acc += LEVELS[level - 1].mix[m];
        if (r < acc)
            return m;
    }
    return M_CONTA;
}

/* reading order, so the guided count sweeps left to right, top to bottom */
static int cmp_reading(const void *a, const void *b)
{
    const int *p = a, *q = b;
    int ra = p[1] / 28, rb = q[1] / 28;
    return ra != rb ? ra - rb : p[0] - q[0];
}

/* rows of five (four for big objects), with an extra gap after ten */
static void layout_grid(quiz_t *q, int n)
{
    int per_row = S.osz > OBJ_SMALL ? 4 : 5;
    int rows = (n + per_row - 1) / per_row, big_gap = rows > 2 ? 8 : 0;
    int total_h = rows * S.osz + (rows - 1) * 6 + big_gap;
    int x0 = q->panel_cx - (per_row * S.osz + (per_row - 1) * 10) / 2;
    int y0 = q->panel_cy - total_h / 2;
    for (int i = 0; i < n; i++) {
        int r = i / per_row, c = i % per_row;
        S.px[i] = x0 + c * (S.osz + 10);
        S.py[i] = y0 + r * (S.osz + 6) + (r >= 2 ? big_gap : 0);
        S.grp[i] = 0;
    }
}

static bool layout_scatter(quiz_t *q, int n)
{
    int xs[MAXN][2];
    for (int i = 0; i < n; i++) {
        int tries = 0;
        for (;;) {
            int x = rng_range(q->panel_cx - 100, q->panel_cx + 100 - S.osz);
            int y = rng_range(q->panel_cy - 58, q->panel_cy + 58 - S.osz);
            int d = S.osz + 8; /* never touching: each object is easy to point at */
            bool ok = true;
            for (int j = 0; j < i && ok; j++) {
                int dx = x - xs[j][0], dy = y - xs[j][1];
                ok = dx * dx + dy * dy >= d * d;
            }
            if (ok) {
                xs[i][0] = x;
                xs[i][1] = y;
                break;
            }
            if (++tries > 400)
                return false;
        }
    }
    qsort(xs, (size_t)n, sizeof(xs[0]), cmp_reading);
    for (int i = 0; i < n; i++) {
        S.px[i] = xs[i][0];
        S.py[i] = xs[i][1];
        S.grp[i] = 0;
    }
    return true;
}

/* up to six big objects on the spots of a die, or in a row (one time in three), in reading order */
static void layout_dice(quiz_t *q, int n)
{
    static const signed char DIE[7][6][2] = {
        {{0}},
        {{0, 0}},
        {{-50, -26}, {50, 26}},
        {{-60, -36}, {0, 0}, {60, 36}},
        {{-36, -28}, {36, -28}, {-36, 28}, {36, 28}},
        {{-54, -34}, {54, -34}, {0, 0}, {-54, 34}, {54, 34}},
        {{-60, -28}, {0, -28}, {60, -28}, {-60, 28}, {0, 28}, {60, 28}},
    };
    n = clampi(n, 1, 6);
    bool row = n >= 2 && rng_range(0, 2) == 0;
    for (int i = 0; i < n; i++) {
        int dx = DIE[n][i][0], dy = DIE[n][i][1];
        if (row) { /* one row of up to four, or two rows of three */
            int per = n <= 4 ? n : 3, r = i / per, c = i % per, in = r == 0 ? imin(per, n) : n - per;
            dx = (int)((c - (in - 1) / 2.0f) * 56);
            dy = n <= 4 ? 0 : (r ? 28 : -28);
        }
        S.px[i] = q->panel_cx + dx - OBJ_HUGE / 2;
        S.py[i] = q->panel_cy + dy - OBJ_HUGE / 2;
        S.grp[i] = 0;
    }
}

/* a small block of k objects (2 per row) centred on (cx, cy), group g */
static void layout_group(int k, int cx, int cy, int g)
{
    int sz = S.osz, rows = (k + 1) / 2, h = rows * (sz + 4) - 4;
    for (int i = 0; i < k && S.n < MAXN; i++) {
        int r = i / 2, c = i % 2, in_row = (i / 2 == rows - 1 && k % 2) ? 1 : 2;
        int w = in_row * (sz + 4) - 4;
        S.px[S.n] = cx - w / 2 + c * (sz + 4);
        S.py[S.n] = cy - h / 2 + r * (sz + 4);
        S.grp[S.n] = g;
        S.n++;
    }
}

/* three options around the answer, at distance 1 or 2, in ascending order; the
   answer's place (smallest, middle, largest) is drawn first, so that the middle
   card - where the highlight starts - is not the right one more often */
static void number_options(quiz_t *q, int answer, int lo, int hi)
{
    int pos[3], np = 0;
    if (answer + 2 <= hi)
        pos[np++] = 0;
    if (answer - 1 >= lo && answer + 1 <= hi)
        pos[np++] = 1;
    if (answer - 2 >= lo)
        pos[np++] = 2;
    int p = np ? pos[rng_range(0, np - 1)] : 1, o[3];
    if (p == 0) {
        o[0] = answer, o[1] = answer + 1, o[2] = answer + 2;
    } else if (p == 2) {
        o[0] = answer - 2, o[1] = answer - 1, o[2] = answer;
    } else {
        o[0] = answer - ((answer - 2 >= lo && rng_range(0, 1)) ? 2 : 1);
        o[1] = answer;
        o[2] = answer + ((answer + 2 <= hi && rng_range(0, 1)) ? 2 : 1);
    }
    for (int i = 0; i < 3; i++)
        S.opt[i] = o[i];
    q->correct = p;
}

static void setup_conta(quiz_t *q)
{
    int level = S.level;
    do
        S.count = rng_range(LEVELS[level - 1].lo, LEVELS[level - 1].hi);
    while (S.count == S.last_count);
    S.last_count = S.count;
    number_options(q, S.count, 1, 20);
    S.n = S.count;
    S.osz = LEVELS[level - 1].grid ? OBJ_SMALL : (S.count <= 6 ? OBJ_HUGE : OBJ_BIG);
    if (S.osz == OBJ_HUGE)
        layout_dice(q, S.count);
    else if (LEVELS[level - 1].grid || !layout_scatter(q, S.count))
        layout_grid(q, S.count);
    snprintf(S.subject, sizeof(S.subject), "conta:%s", OBJ_NAMES[S.obj]);
}

static void setup_piu(quiz_t *q)
{
    /* level 2: the biggest group is clearly bigger; later just one more */
    int gap = S.level <= 2 ? 3 : 1;
    int big = rng_range(gap + 1, 8), a, b;
    do {
        a = rng_range(1, big - gap);
        b = rng_range(1, big - gap);
    } while (a == b && S.level > 2 && big - gap > 1);
    q->correct = rng_range(0, 2);
    int k = 0, other[2] = {a, b};
    for (int i = 0; i < 3; i++)
        S.group_n[i] = i == q->correct ? big : other[k++];
    S.n = 0;
    S.osz = OBJ_SMALL;
    for (int g = 0; g < 3; g++)
        layout_group(S.group_n[g], quiz_card_cx(g), q->panel_cy, g);
    snprintf(S.subject, sizeof(S.subject), "piu:%d-%d-%d", S.group_n[0], S.group_n[1], S.group_n[2]);
}

static void setup_line(quiz_t *q, bool after)
{
    int top = S.level >= 5 ? 20 : 10;
    int answer;
    if (after) { /* s s+1 s+2 ? */
        S.line0 = rng_range(1, top - 3);
        answer = S.line0 + 3;
    } else {     /* ? s+1 s+2 s+3 */
        S.line0 = rng_range(1, top - 3);
        answer = S.line0;
    }
    number_options(q, answer, 1, 20);
    S.n = 0;
    snprintf(S.subject, sizeof(S.subject), "%s:%d", after ? "dopo" : "prima", after ? S.line0 + 2 : S.line0 + 1);
}

static void setup_insieme(quiz_t *q)
{
    int top = S.level >= 5 ? 10 : 8, a, b;
    do {
        a = rng_range(1, 5);
        b = rng_range(1, 5);
    } while (a + b > top);
    S.group_n[0] = a;
    S.group_n[1] = b;
    S.count = a + b;
    number_options(q, S.count, 1, 20);
    S.n = 0;
    S.osz = OBJ_BIG; /* two little heaps, big enough to count (at most 5 each: 3 rows of 36) */
    layout_group(a, q->panel_cx - 58, q->panel_cy, 0);
    layout_group(b, q->panel_cx + 58, q->panel_cy, 1);
    snprintf(S.subject, sizeof(S.subject), "insieme:%d+%d", a, b);
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        S.last_obj = S.last_count = -1;
    S.level = level;
    S.mode = pick_mode(level);
    do
        S.obj = rng_range(0, NOBJ - 1);
    while (S.obj == S.last_obj);
    S.last_obj = S.obj;
    S.hint_i = -1;
    S.show_counts = false;
    S.line_filled = false;
    q->card = QUIZ_CARD_NUMBER;
    switch (S.mode) {
    case M_PIU: setup_piu(q); break;
    case M_DOPO: setup_line(q, true); break;
    case M_PRIMA: setup_line(q, false); break;
    case M_INSIEME: setup_insieme(q); break;
    default: setup_conta(q); break;
    }
}

static bool prepare(quiz_t *q)
{
    if (S.mode == M_DOPO || S.mode == M_PRIMA) { /* the boxes pop in left to right */
        if (q->t % 8 == 1 && q->t / 8 < 4)
            sfx("pop");
        return q->t >= 4 * 8 + 10;
    }
    int landed = q->t - 9; /* object k lands at t = 9 + 4k */
    if (landed >= 0 && landed % 4 == 0 && landed / 4 < S.n)
        sfx("pop");
    return q->t >= S.n * 4 + 14;
}

static void ask(quiz_t *q)
{
    char id[24];
    switch (S.mode) {
    case M_PIU:
        say("piu_q");
        break;
    case M_DOPO:
        say("dopo_q");
        say_number(S.line0 + 2, true);
        break;
    case M_PRIMA:
        say("prima_q");
        say_number(S.line0 + 1, true);
        break;
    case M_INSIEME:
        say("insieme_q");
        break;
    default:
        snprintf(id, sizeof(id), "q_%s", OBJ_NAMES[S.obj]);
        say(id);
        break;
    }
}

static void hover(quiz_t *q, int i)
{
    if (S.mode != M_PIU) /* in "di più" the number would give the answer away */
        say_number(S.opt[i], false);
}

static bool hint_count(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("aiuto");
        return false;
    }
    if (++S.hint_i < S.n) {
        if (S.hint_i == 2)
            BOT("hint_counting\n");
        say_number(S.hint_i + 1, false);
        sfx("pop");
        return false;
    }
    S.hint_i = S.n - 1;
    say("aiuto_scegli");
    return true;
}

static bool hint(quiz_t *q)
{
    switch (S.mode) {
    case M_PIU: /* how many in each group, then "the biggest number" */
        if (q->hint_step == 0) {
            S.show_counts = true;
            say("piu_aiuto");
            return false;
        }
        if (q->hint_step <= 3) {
            say_number(S.group_n[q->hint_step - 1], false);
            return false;
        }
        say("piu_scegli");
        return true;
    case M_DOPO:
    case M_PRIMA: /* read the line aloud, the missing number included */
        if (q->hint_step < 4) {
            say_number(S.line0 + q->hint_step, false);
            if (q->hint_step == (S.mode == M_DOPO ? 3 : 0))
                S.line_filled = true;
            return false;
        }
        say("aiuto_scegli");
        return true;
    default:
        return hint_count(q);
    }
}

static void right(quiz_t *q)
{
    if (S.mode == M_PIU)
        S.show_counts = true;
    if (S.mode == M_DOPO || S.mode == M_PRIMA)
        S.line_filled = true;
    /* the number once more after the praise, "Bravissima! ...Sei!": the count and its name together
       (0.14; "dove ce ne sono di più" has no number to say) */
    if (S.mode != M_PIU && !q->duel)
        say_number(S.opt[q->correct], true);
}

static void draw_objects(quiz_t *q)
{
    char name[24];
    snprintf(name, sizeof(name), "%s_%s", S.osz == OBJ_BIG ? "objb" : "obj", OBJ_NAMES[S.obj]);
    const sprite_t *spr = gfx_sprite(name);
    int k = S.osz == OBJ_HUGE ? 2 : 1; /* the big ones: the small drawing, twice as big */
    for (int i = 0; i < S.n; i++) {
        int x = S.px[i], y = S.py[i];
        if (q->state == Q_PREPARE) {
            int dt = q->t - 1 - i * 4;
            if (dt < 0)
                continue;
            if (dt < 8)
                y -= (8 - dt) * 2; /* drop in */
            else if (dt < 11)
                y -= 1; /* tiny bounce */
        } else if (q->state == Q_RIGHT && q->t < 70) {
            y -= ((q->t + i * 3) % 16) < 8 ? 2 : 0; /* happy hop */
        }
        if (q->state == Q_HINT && i == S.hint_i && voice_busy())
            y -= 3; /* the object being counted pops up */
        gfx_blit_scaled(spr, x, y, k, 0);
        if (S.hint_i >= 0 && i <= S.hint_i && q->state != Q_PREPARE)
            gfx_number_small(i + 1, x + S.osz / 2, y - 3);
    }
    if (S.mode == M_INSIEME && q->state != Q_PREPARE) {
        const sprite_t *p = gfx_sprite("plus");
        gfx_blit(p, q->panel_cx - p->w / 2, q->panel_cy - p->h / 2, 0);
    }
}

static void draw_line(quiz_t *q)
{
    int x0 = q->panel_cx - (4 * BOX_STEP - (BOX_STEP - BOX_W)) / 2, y = q->panel_cy - BOX_H / 2;
    int gap = S.mode == M_DOPO ? 3 : 0;
    for (int i = 0; i < 4; i++) {
        if (q->state == Q_PREPARE && q->t < 1 + i * 8)
            continue;
        int x = x0 + i * BOX_STEP, dy = 0;
        if (q->state == Q_HINT && voice_busy() && q->hint_step - 1 == i)
            dy = -3; /* the number being read */
        if (q->state == Q_RIGHT && q->t < 90)
            dy = ((q->t / 4 + 4 - i) % 8) < 2 ? -3 : 0;
        bool missing = i == gap;
        gfx_blit(gfx_sprite(missing ? "nbox_q" : "nbox"), x, y + dy, 0);
        if (missing && !S.line_filled) {
            const sprite_t *qm = gfx_sprite("qmark");
            gfx_blit(qm, x + (BOX_W - qm->w) / 2, y + (BOX_H - qm->h) / 2 + dy - swing((int)G.frame, 60, 1), 0);
        } else {
            gfx_number_big(S.line0 + i, x + BOX_W / 2, y + BOX_H / 2 + dy, missing ? NUM_SELECTED : NUM_NORMAL);
        }
    }
}

static void draw(quiz_t *q)
{
    if (S.mode == M_DOPO || S.mode == M_PRIMA)
        draw_line(q);
    else
        draw_objects(q);
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    int num_style = style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL);
    if (S.mode == M_PIU) {
        if (S.show_counts) {
            gfx_number_big(S.group_n[i], cx, cy, num_style);
        } else { /* the card points at the group above it */
            const sprite_t *a = gfx_sprite("arrow_up");
            if (style == CARD_OFF)
                gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
            gfx_blit(a, cx - a->w / 2, cy - a->h / 2, 0);
            gfx_set_tint(0, 0);
        }
        return;
    }
    gfx_number_big(S.opt[i], cx, cy, num_style);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    snprintf(buf, n, "%d", S.mode == M_PIU ? S.group_n[i] : S.opt[i]);
}

static const char *subject(quiz_t *q) { return S.subject; }

const quiz_def_t QUIZ_CONTA = {
    .name = "conta",
    .game = GAME_CONTA,
    .bg = "bg_bacheca", /* 0.12: a meadow, the question on a noticeboard */
    .intro = "conta_intro",
    .card = QUIZ_CARD_NUMBER,
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

static void enter(void) { quiz_enter(&QUIZ_CONTA); }

const scene_t SCENE_CONTA = {enter, quiz_update, quiz_draw};

