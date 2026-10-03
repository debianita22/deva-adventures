/* Deva's Awesome Adventures - "Il negozio" (0.11.0): coins and prices.
 *
 * The toy shop of the Regno dei Giocattoli: every toy has its price on a
 * little sign, and Deva pays with coins. Two kinds of question:
 *   paga  "Il trenino costa... sei. Quali monete ci vogliono?" - three purses
 *         on the cards, the right one makes exactly the price
 *   vale  coins on the counter: "Quante monete ci sono?" / "Quanto fanno,
 *         tutte insieme?" -> the number
 * Levels: 1 coins of 1, prices 2..5, the sign shows the coins as dots too;
 *   2 coins of 1 up to 8, and "vale"; 3 the silver coin worth 2 (up to 10);
 *   4 the golden coin worth 5 (7 to 12); 5 up to 15. The first time a silver
 *   or a golden coin shows up in a round, the voice says what it is worth.
 * Guided help: the coins go on the counter and are counted aloud, each one
 *   adding its value ("cinque... sette... otto!"), the running total over it.
 * Right answer: the coins fly into the till, ding, and the toy flies to Deva.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

#define MAXC 8     /* coins in a purse at most (all of 1) */
#define MAXC_MIX 6 /* ...when there are silver or golden ones */

static const char *const TOYS[] = {"trenino", "orsetto", "palla",      "aquilone", "trottola",
                                   "robot",   "dinosauro", "palloncino", "corona", "tamburo"};
#define NTOYS ARRAY_LEN(TOYS)

enum { M_PAGA, M_VALE };

static const struct {
    int lo, hi; /* the price, or the value of the coins */
    int kinds;  /* 1: coins of 1; 2: and of 2; 5: and of 5 */
    int vale;   /* percent of "vale" questions */
} LEVELS[5] = { /* with the golden coin the prices start at 7: the cards next to it can have one too */
    {2, 5, 1, 0}, {3, 8, 1, 40}, {3, 10, 2, 50}, {7, 12, 5, 50}, {8, 15, 5, 50},
};

typedef struct {
    int n;
    uint8_t v[MAXC]; /* the biggest first */
} purse_t;

/* the counter (positions from the centre of the panel, see bg_negozio) */
#define TOY_DX (-62)  /* the toy for sale, at 2x, standing on the counter */
#define SIGN_DX 6     /* its price sign */
#define TILL_DX 74    /* the till */
#define TOP_DY 10     /* the top of the counter: where things stand */
#define ROW_DY 42     /* the coins lying on the counter */
#define BAG_DX (-84)  /* "vale": the purse they come out of */
/* the right answer: the coins fly to the till, then the toy flies to Deva */
#define FLY0 6
#define FLY_GAP 4
#define FLY_LEN 18
#define TOY_LEN 24

static struct {
    int mode, level, kinds;
    int toy, last_toy, value, last_value;
    purse_t purse[3]; /* paga: the purse on each card; vale: purse[0] is on the counter */
    int opt[3];       /* the value of each card */
    int spread;       /* guided help (paga): frames since the coins came to the counter, -1 = not yet */
    int counted;      /* guided help: coins counted aloud */
    bool told2, told5;
    int open;         /* the drawer of the till is open (frames left) */
    int toy_fly;      /* frames since the toy left for Deva, -1 = not yet */
    char subject[40];
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

static int min_coins(int value, int fives)
{
    int r = value - 5 * fives;
    return fives + r / 2 + r % 2;
}

/* a natural handful of coins for this value: with kinds 2 at least one silver
   coin, with kinds 5 at least one golden coin (when the value allows it) */
static void make_purse(purse_t *p, int value, int kinds)
{
    int fives = 0, twos = 0, cap = kinds == 1 ? MAXC : MAXC_MIX;
    if (kinds == 5 && value >= 5) {
        int cand[4], nc = 0;
        for (int f = 1; f <= value / 5 && nc < 4; f++)
            if (min_coins(value, f) <= cap)
                cand[nc++] = f;
        fives = nc ? cand[rng_range(0, nc - 1)] : value / 5;
    }
    int rest = value - 5 * fives;
    if (kinds >= 2 && rest >= 2) {
        int lo = fives + rest - cap, hi = rest / 2; /* coins = fives + rest - twos <= cap */
        if (kinds == 2 && lo < 1)
            lo = 1;
        if (lo < 0)
            lo = 0;
        twos = lo > hi ? hi : rng_range(lo, hi);
    }
    int ones = rest - 2 * twos;
    p->n = 0;
    for (int i = 0; i < fives && p->n < MAXC; i++)
        p->v[p->n++] = 5;
    for (int i = 0; i < twos && p->n < MAXC; i++)
        p->v[p->n++] = 2;
    for (int i = 0; i < ones && p->n < MAXC; i++)
        p->v[p->n++] = 1;
}

static int running(const purse_t *p, int k)
{
    int s = 0;
    for (int i = 0; i < k && i < p->n; i++)
        s += p->v[i];
    return s;
}

static void purse_str(const purse_t *p, char *buf, size_t n)
{
    size_t len = 0;
    buf[0] = 0;
    for (int i = 0; i < p->n && len + 3 < n; i++)
        len += (size_t)snprintf(buf + len, n - len, i ? "+%d" : "%d", p->v[i]);
}

/* three options around the answer, at distance 1 or 2, in ascending order (as in Conta) */
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

static void setup(quiz_t *q, int level)
{
    if (q->question == 0) {
        S.last_toy = S.last_value = -1;
        S.told2 = S.told5 = false;
    }
    S.level = level;
    S.kinds = LEVELS[level - 1].kinds;
    S.mode = rng_range(0, 99) < LEVELS[level - 1].vale ? M_VALE : M_PAGA;
    do
        S.value = rng_range(LEVELS[level - 1].lo, LEVELS[level - 1].hi);
    while (S.value == S.last_value);
    S.last_value = S.value;
    do
        S.toy = rng_range(0, NTOYS - 1);
    while (S.toy == S.last_toy);
    S.last_toy = S.toy;
    S.spread = -1;
    S.counted = 0;
    S.open = 0;
    S.toy_fly = -1;
    number_options(q, S.value, 1, S.kinds == 1 ? MAXC : 20);
    if (S.mode == M_PAGA) {
        q->card = QUIZ_CARD_PICTURE;
        for (int i = 0; i < 3; i++)
            make_purse(&S.purse[i], S.opt[i], S.kinds);
        snprintf(S.subject, sizeof(S.subject), "paga:%s:%d", TOYS[S.toy], S.value);
    } else {
        char coins[24];
        q->card = QUIZ_CARD_NUMBER;
        make_purse(&S.purse[0], S.value, S.kinds);
        purse_str(&S.purse[0], coins, sizeof(coins));
        snprintf(S.subject, sizeof(S.subject), "vale:%s", coins);
    }
}

/* ------------------------------------------------------------------ where things are */
static int row_step(const purse_t *p) { return p->n && p->v[0] > 1 ? 32 : 24; } /* 2x, on the counter */

static void row_xy(quiz_t *q, const purse_t *p, int i, int *x, int *y)
{
    int cx = q->panel_cx + (S.mode == M_VALE ? 12 : 0), step = row_step(p);
    *x = cx - (p->n - 1) * step / 2 + i * step;
    *y = q->panel_cy + ROW_DY;
}

/* on a card (1x): rows of four coins of 1, or three with bigger ones */
static void card_xy(const purse_t *p, int i, int cx, int cy, int *x, int *y)
{
    bool ones = !p->n || p->v[0] == 1;
    int per = ones ? 4 : 3, step = ones ? 12 : 16;
    int rows = p->n > per ? 2 : 1, in0 = (p->n + rows - 1) / rows;
    int r = i / in0, c = i % in0, cnt = r == 0 ? in0 : p->n - in0;
    *x = cx - (cnt - 1) * step / 2 + c * step;
    *y = cy - (rows - 1) * step / 2 + r * step;
}

static const sprite_t *coin(int v)
{
    return gfx_sprite(v >= 5 ? "coin_5" : (v == 2 ? "coin_2" : "coin_1"));
}

static void draw_coin(int v, int x, int y, int k)
{
    const sprite_t *s = coin(v);
    gfx_blit_scaled(s, x - s->w * k / 2, y - s->h * k / 2, k, 0);
}

static int fly_start(int k) { return FLY0 + k * FLY_GAP; }

static const purse_t *paid(quiz_t *q) { return S.mode == M_PAGA ? &S.purse[q->correct] : &S.purse[0]; }

/* ------------------------------------------------------------------ the flow */
static bool prepare(quiz_t *q)
{
    if (S.mode == M_PAGA) { /* the toy drops on the counter, then its sign pops up */
        if (q->t == 12)
            sfx("pop");
        if (q->t == 18) {
            sfx("pop");
            fx_twinkle(q->panel_cx + SIGN_DX + 18, q->panel_cy + TOP_DY - 40, 0);
        }
        return q->t > 28;
    }
    int k = q->t - 6; /* "vale": the coins jump out of the purse, one by one */
    if (k >= 0 && k % 5 == 4 && k / 5 < S.purse[0].n)
        sfx("coin");
    return q->t >= S.purse[0].n * 5 + 16;
}

static void tell_coins(void)
{
    bool two = false, five = false;
    for (int c = 0; c < (S.mode == M_PAGA ? 3 : 1); c++)
        for (int i = 0; i < S.purse[c].n; i++) {
            two |= S.purse[c].v[i] == 2;
            five |= S.purse[c].v[i] == 5;
        }
    if (two && !S.told2) {
        S.told2 = true;
        say_then("neg_vale_2");
    }
    if (five && !S.told5) {
        S.told5 = true;
        say_then("neg_vale_5");
    }
}

static void ask(quiz_t *q)
{
    char id[32];
    if (S.mode == M_PAGA) {
        snprintf(id, sizeof(id), "neg_c_%s", TOYS[S.toy]);
        say(id);
        say_number(S.value, true);
        say_then("neg_paga");
    } else {
        say(S.kinds == 1 ? "neg_quante" : "neg_quanto");
    }
    tell_coins();
}

static void hover(quiz_t *q, int i)
{
    if (S.mode == M_VALE)
        say_number(S.opt[i], false);
    /* a purse says nothing: its value would give the answer away */
}

static void right(quiz_t *q)
{
    static const char *const THANKS[] = {"neg_grazie_1", "neg_grazie_2", "neg_grazie_3"};
    if (S.mode == M_PAGA && !q->duel)
        say_then(THANKS[rng_range(0, ARRAY_LEN(THANKS) - 1)]);
}

static bool hint(quiz_t *q)
{
    const purse_t *p = paid(q);
    if (q->hint_step == 0) {
        say("neg_aiuto");
        if (S.mode == M_PAGA) { /* the coins of the right purse go on the counter */
            S.spread = 0;
            sfx("whoosh");
        }
        S.counted = 0;
        return false;
    }
    if (S.counted < p->n) {
        S.counted++;
        say_number(running(p, S.counted), false);
        sfx("coin");
        if (S.counted == 2)
            BOT("hint_counting\n");
        return false;
    }
    say(S.mode == M_PAGA ? "neg_paga_queste" : "aiuto_scegli");
    return true;
}

static void tick(quiz_t *q)
{
    if (S.spread >= 0)
        S.spread++;
    if (S.open > 0)
        S.open--;
    if (S.toy_fly >= 0 && ++S.toy_fly == TOY_LEN && !q->duel)
        fx_hearts(G.hero.x + 4, G.hero.y - 70, 3); /* Deva has her toy */
    if (q->state != Q_RIGHT || S.mode != M_PAGA)
        return;
    const purse_t *p = paid(q);
    for (int k = 0; k < p->n; k++)
        if (q->t == fly_start(k) + FLY_LEN)
            sfx("coin");
    int last = fly_start(p->n - 1) + FLY_LEN;
    if (q->t == last + 2) {
        sfx("cassa");
        S.open = 40;
        fx_sparkles(q->panel_cx + TILL_DX, q->panel_cy + TOP_DY - 30, 12, 8);
    }
    if (q->t == last + 12)
        S.toy_fly = 0;
}

/* ------------------------------------------------------------------ drawing */
static void draw_sign(quiz_t *q, int dy)
{
    const sprite_t *s = gfx_sprite("neg_cartello");
    int cx = q->panel_cx + SIGN_DX, x = cx - s->w / 2, y = q->panel_cy + TOP_DY - s->h + dy;
    bool dots = S.level == 1 && S.kinds == 1;
    gfx_blit(s, x, y, 0); /* the board: rows 0..37 of the sprite (tools/art/games5.py) */
    gfx_number_big(S.value, cx, y + (dots ? 13 : 18), NUM_NORMAL);
    if (dots) { /* level 1: the price as coins too */
        const sprite_t *d = gfx_sprite("neg_pallino");
        int step = d->w + 1, x0 = cx - (S.value * step - 1) / 2;
        for (int i = 0; i < S.value; i++)
            gfx_blit(d, x0 + i * step, y + 26, 0);
    }
}

static void draw_till(quiz_t *q)
{
    const sprite_t *s = gfx_sprite(S.open > 0 ? "neg_cassa_aperta" : "neg_cassa");
    gfx_blit(s, q->panel_cx + TILL_DX - s->w / 2, q->panel_cy + TOP_DY - s->h, 0);
}

static void draw_toy(quiz_t *q)
{
    char name[32];
    snprintf(name, sizeof(name), "pic_%s", TOYS[S.toy]);
    const sprite_t *s = gfx_sprite(name);
    int x = q->panel_cx + TOY_DX - s->w, y = q->panel_cy + TOP_DY - s->h * 2;
    if (q->state == Q_PREPARE && q->t < 12) {
        y -= (12 - q->t) * (12 - q->t); /* it drops from above */
    } else if (q->state == Q_PREPARE && q->t < 16) {
        y -= 2; /* a little bounce */
    } else if (S.toy_fly >= 0) {
        if (S.toy_fly >= TOY_LEN || q->duel) {
            if (!q->duel)
                return; /* Deva has it */
            y -= ((q->t / 5) & 1) * 3; /* in a duel it just hops */
        } else { /* to Deva, in an arc, getting smaller */
            int u = S.toy_fly * 256 / TOY_LEN;
            int x0 = x + s->w, y0 = y + s->h, x1 = G.hero.x + 4, y1 = G.hero.y - 46;
            int cx = x0 + (x1 - x0) * u / 256, cy = y0 + (y1 - y0) * u / 256 - 40 * u * (256 - u) / 16384;
            int k = u < 128 ? 2 : 1;
            gfx_blit_scaled(s, cx - s->w * k / 2, cy - s->h * k / 2, k, 0);
            return;
        }
    } else if (q->state == Q_RIGHT) {
        y -= ((q->t / 5) & 1) * 2; /* happy: it is going home with Deva */
    }
    gfx_blit_scaled(s, x, y, 2, 0);
}

/* the coins of the purse on the counter, with the running total over the counted ones */
static void draw_row(quiz_t *q, const purse_t *p, int from_card)
{
    for (int i = 0; i < p->n; i++) {
        int x, y;
        row_xy(q, p, i, &x, &y);
        if (S.mode == M_VALE && q->state == Q_PREPARE) { /* out of the purse, one by one */
            int dt = q->t - 6 - i * 5;
            if (dt < 0)
                continue;
            if (dt < 10) {
                int bx = q->panel_cx + BAG_DX, by = q->panel_cy + TOP_DY - 14;
                int u = dt * 256 / 10;
                x = bx + (x - bx) * u / 256;
                y = by + (y - by) * u / 256 - 26 * u * (256 - u) / 16384;
                draw_coin(p->v[i], x, y, 1);
                continue;
            }
        }
        if (from_card >= 0 && S.spread < 14) { /* guided help: from the card to the counter */
            int x0, y0, u = S.spread * 256 / 14;
            card_xy(p, i, quiz_card_cx(from_card), quiz_card_cy(), &x0, &y0);
            draw_coin(p->v[i], x0 + (x - x0) * u / 256, y0 + (y - y0) * u / 256 - 20 * u * (256 - u) / 16384, 1);
            continue;
        }
        int dy = 0;
        if (q->state == Q_HINT && i == S.counted - 1 && voice_busy())
            dy = -4; /* the coin being counted jumps */
        else if (q->state == Q_RIGHT && S.mode == M_VALE && q->t < 80)
            dy = ((q->t + i * 3) % 16) < 8 ? -2 : 0;
        draw_coin(p->v[i], x, y + dy, 2);
        if (i < S.counted) {
            const sprite_t *c = coin(p->v[i]);
            gfx_number_small(running(p, i + 1), x, y - c->h - 6 + dy);
        }
    }
}

static void draw(quiz_t *q)
{
    if (S.mode == M_VALE) {
        const sprite_t *bag = gfx_sprite("neg_borsellino");
        gfx_blit(bag, q->panel_cx + BAG_DX - bag->w / 2, q->panel_cy + TOP_DY - bag->h, 0);
        draw_till(q);
        draw_row(q, &S.purse[0], -1);
        return;
    }
    draw_till(q);
    if (q->state != Q_PREPARE || q->t >= 18) /* the sign pops up after the toy */
        draw_sign(q, q->state == Q_PREPARE && q->t < 21 ? 2 : 0);
    draw_toy(q);
    if (S.spread >= 0)
        draw_row(q, &S.purse[q->correct], q->correct);
    if (q->state == Q_RIGHT) { /* the coins fly from the card into the till */
        const purse_t *p = paid(q);
        int tx = q->panel_cx + TILL_DX, ty = q->panel_cy + TOP_DY - 30;
        for (int k = 0; k < p->n; k++) {
            int dt = q->t - fly_start(k);
            if (dt < 0 || dt >= FLY_LEN)
                continue;
            int x0, y0, u = dt * 256 / FLY_LEN;
            card_xy(p, k, quiz_card_cx(q->correct), quiz_card_cy(), &x0, &y0);
            draw_coin(p->v[k], x0 + (tx - x0) * u / 256, y0 + (ty - y0) * u / 256 - 34 * u * (256 - u) / 16384, 1);
        }
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (S.mode == M_VALE) {
        gfx_number_big(S.opt[i], cx, cy, style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL));
        return;
    }
    const purse_t *p = &S.purse[i];
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    for (int k = 0; k < p->n; k++) {
        if (q->state == Q_RIGHT && i == q->correct && q->t >= fly_start(k))
            continue; /* on its way to the till */
        int x, y;
        card_xy(p, k, cx, cy, &x, &y);
        draw_coin(p->v[k], x, y, 1);
    }
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    if (S.mode == M_PAGA)
        purse_str(&S.purse[i], buf, n);
    else
        snprintf(buf, n, "%d", S.opt[i]);
}

static const char *subject(quiz_t *q) { return S.subject; }

const quiz_def_t QUIZ_NEGOZIO = {
    .name = "negozio",
    .game = GAME_NEGOZIO,
    .bg = "bg_negozio",
    .intro = "neg_intro",
    .card = QUIZ_CARD_PICTURE,
    .setup = setup,
    .prepare = prepare,
    .tick = tick,
    .ask = ask,
    .hover = hover,
    .right = right,
    .hint = hint,
    .draw = draw,
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void) { quiz_enter(&QUIZ_NEGOZIO); }

const scene_t SCENE_NEGOZIO = {enter, quiz_update, quiz_draw};
