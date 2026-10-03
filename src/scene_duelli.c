/* Deva's Awesome Adventures - duel questions for the games that are not card
 * games, so that in the second adventure every game has its spell:
 *
 *   QUIZ_PASSI    (Balla con me)  the monster does one or two steps: which arrows?
 *   QUIZ_TAMBURO  (Batti il ritmo) the monster beats its drum: which rhythm, as
 *                 dots close together (quick) or far apart (slow)? Moving over
 *                 a card plays its rhythm.
 *   QUIZ_DOVE     (Sopra e sotto) "trova la stella sotto il tavolo": which picture?
 *   QUIZ_SPARITO  (Memory)        make-up in a row, poof: which one is gone?
 *
 * They run on the quiz engine like the card games (three cards, voice, gentle
 * help after two mistakes) and move the level of their own game.
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
#include "save.h"
#include "story.h"
#include "ui.h"

/* the monster of the duel (a friendly one, when asked outside a duel) */
static const char *foe_sprite(const quiz_t *q, char form)
{
    if (!q->duel)
        return "mo_ciuffone_b";
    return story_foe_sprite(clampi(story_chapter(), 0, CH_COUNT - 1), form);
}

static void off_tint(int style)
{
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
}

/* small shapes for the card pictures */
static void dot(int cx, int cy, uint16_t c)
{
    gfx_fill_rect(cx - 1, cy - 3, 3, 7, c);
    gfx_fill_rect(cx - 3, cy - 1, 7, 3, c);
    gfx_fill_rect(cx - 2, cy - 2, 5, 5, c);
}

/* ------------------------------------------------------------------ QUIZ_PASSI */

enum { D_SU, D_GIU, D_SX, D_DX };
static const char *const ARROW[4] = {"arrow_up", "arrow_down", "arrow_left", "arrow_right"};
static const char *const TONE[4] = {"tone_su", "tone_giu", "tone_sx", "tone_dx"};
static const char *const DIR_VOICE[4] = {"dl_su", "dl_giu", "dv_w_sinistra", "dl_destra"};
static const char *const DIR_ID[4] = {"su", "giu", "sx", "dx"};
#define STEP_T 60    /* frames of one step of the monster: it goes, stays a moment, comes back */
#define STEP_SLOW 90 /* the same, in the guided help */

static struct {
    int n, mv[2], opt[3][2];
    int show_t;       /* frames into the show, -1 = still */
    int step_t;       /* frames of one step in this show */
} PS;

static bool same_steps(const int *a, const int *b) { return a[0] == b[0] && (PS.n < 2 || a[1] == b[1]); }

static void passi_setup(quiz_t *q, int level)
{
    PS.n = level >= 3 ? 2 : 1;
    PS.mv[0] = rng_range(0, 3);
    do
        PS.mv[1] = rng_range(0, 3);
    while (PS.mv[1] == PS.mv[0]);
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    int made = 0, d[2][2];
    while (made < 2) { /* two other steps (two other pairs), all different */
        int c[2];
        rng_pair(0, 3, 0, 3, &c[0], &c[1]);
        if (PS.n == 2 && made == 0 && rng_range(0, 1)) { /* the same steps, the other way round */
            c[0] = PS.mv[1];
            c[1] = PS.mv[0];
        }
        if (PS.n == 2 && c[0] == c[1])
            continue;
        if (same_steps(c, PS.mv) || (made == 1 && same_steps(c, d[0])))
            continue;
        d[made][0] = c[0];
        d[made][1] = c[1];
        made++;
    }
    for (int i = 0, k = 0; i < 3; i++) {
        const int *src = i == q->correct ? PS.mv : d[k++];
        PS.opt[i][0] = src[0];
        PS.opt[i][1] = src[1];
    }
    PS.show_t = -1;
    PS.step_t = STEP_T;
}

static bool passi_prepare(quiz_t *q)
{
    if (q->t == 1)
        say(q->duel ? story_line("dl_passo_guarda") : "dl_passo_guarda");
    if (PS.show_t < 0) {
        if (q->t > 20 && !voice_busy())
            PS.show_t = 0;
        return false;
    }
    return PS.show_t >= PS.n * PS.step_t + 10;
}

static void passi_tick(quiz_t *q)
{
    if (PS.show_t < 0)
        return;
    if (PS.show_t % PS.step_t == 0 && PS.show_t / PS.step_t < PS.n)
        sfx(TONE[PS.mv[PS.show_t / PS.step_t]]);
    if (PS.show_t < PS.n * PS.step_t + 10)
        PS.show_t++;
}

static void passi_ask(quiz_t *q)
{
    if (q->state == Q_INPUT) { /* B, or asked again: the monster does its steps again */
        PS.step_t = STEP_T;
        PS.show_t = 0;
    }
    say(PS.n == 1 ? "dl_passo_quale" : "dl_passi_quali");
}

static void passi_hover(quiz_t *q, int i)
{
    say(DIR_VOICE[PS.opt[i][0]]);
    if (PS.n == 2)
        say_then(DIR_VOICE[PS.opt[i][1]]);
}

static bool passi_hint(quiz_t *q)
{
    if (q->hint_step == 0) { /* the steps again, slowly */
        say("balla_guarda");
        PS.step_t = STEP_SLOW;
        PS.show_t = 0;
        q->hint_wait = PS.n * PS.step_t + 10;
        return false;
    }
    passi_ask(q);
    return true;
}

/* one step: the arrow she would press in "Balla con me" picks the card with that
   arrow (then A, as always); an arrow on no card: boop */
static bool passi_key(quiz_t *q)
{
    static const int B[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
    if (PS.n != 1)
        return false;
    for (int d = 0; d < 4; d++) {
        if (!btn_pressed(B[d]))
            continue;
        for (int i = 0; i < 3; i++)
            if (PS.opt[i][0] == d && !q->off[i]) {
                if (i != q->sel)
                    quiz_select(i);
                else
                    passi_hover(q, i);
                return true;
            }
        sfx("boop");
        return true;
    }
    return false;
}

static void passi_draw(quiz_t *q)
{
    const sprite_t *s = gfx_sprite(foe_sprite(q, 'c'));
    int x0 = q->panel_cx - s->w / 2, y0 = q->panel_cy + 44 - s->h, x = x0, y = y0;
    if (PS.show_t >= 0 && PS.show_t < PS.n * PS.step_t) {
        int T = PS.step_t, m = PS.mv[PS.show_t / T], k = PS.show_t % T;
        int out = T / 4, stay = T * 13 / 20, back = T * 9 / 10;
        int a = k < out ? 256 * k / out : k < stay ? 256 : k < back ? 256 * (back - k) / (back - stay) : 0;
        a = a * (512 - a) / 256; /* 0 -> 256, stays, -> 0, quick at first, then softly */
        switch (m) {
        case D_SU: y -= 32 * a / 256; break;
        case D_GIU: y += 16 * a / 256; break;
        case D_SX: x -= 56 * a / 256; break;
        default: x += 56 * a / 256; break;
        }
        if (a > 0) { /* a pale shadow where it was: which way it went is clear */
            gfx_set_tint(COL_CREAM, 190);
            gfx_blit(s, x0, y0, 0);
            gfx_set_tint(0, 0);
        }
    }
    actor_draw(foe_sprite(q, 'c'), x + s->w / 2, y + s->h, 1, 0, 0, 0, 0); /* alive (0.13) */
}

static void passi_card(quiz_t *q, int i, int cx, int cy, int style)
{
    off_tint(style);
    for (int k = 0; k < PS.n; k++) {
        const sprite_t *a = gfx_sprite(ARROW[PS.opt[i][k]]);
        int x = PS.n == 1 ? cx - a->w / 2 : cx - a->w - 1 + k * (a->w + 2);
        gfx_blit(a, x, cy - a->h / 2, 0);
    }
    gfx_set_tint(0, 0);
}

static void passi_value(quiz_t *q, int i, char *buf, size_t n)
{
    if (PS.n == 1)
        snprintf(buf, n, "%s", DIR_ID[PS.opt[i][0]]);
    else
        snprintf(buf, n, "%s-%s", DIR_ID[PS.opt[i][0]], DIR_ID[PS.opt[i][1]]);
}

static const char *passi_subject(quiz_t *q)
{
    static char s[24], v[16];
    passi_value(q, q->correct, v, sizeof(v));
    snprintf(s, sizeof(s), "passi:%s", v);
    return s;
}

const quiz_def_t QUIZ_PASSI = {
    .name = "balla",
    .game = GAME_BALLA,
    .bg = "bg_palco",
    .card = QUIZ_CARD_PICTURE,
    .setup = passi_setup,
    .prepare = passi_prepare,
    .tick = passi_tick,
    .ask = passi_ask,
    .hover = passi_hover,
    .key = passi_key,
    .hint = passi_hint,
    .draw = passi_draw,
    .card_draw = passi_card,
    .card_value = passi_value,
    .subject = passi_subject,
};

/* ------------------------------------------------------------------ QUIZ_TAMBURO */

#define GAP_S 14 /* frames between two quick beats */
#define GAP_L 30 /* ...two slow ones */
#define MAX_BEATS 4

typedef struct {
    int n;                 /* beats */
    char gap[MAX_BEATS];   /* 'S' quick, 'L' slow: n - 1 of them */
} rhythm_t;

static struct {
    rhythm_t opt[3];
    const rhythm_t *play;  /* being played, NULL = none */
    int play_t, beat_t;    /* frames into it; frames since the last beat (the drum pulses) */
    bool shown;
    bool replay;           /* the monster's rhythm again when the voice ends (B) */
} TB;

static int rhythm_len(const rhythm_t *r)
{
    int t = 0;
    for (int i = 0; i < r->n - 1; i++)
        t += r->gap[i] == 'L' ? GAP_L : GAP_S;
    return t;
}

static bool same_rhythm(const rhythm_t *a, const rhythm_t *b)
{
    return a->n == b->n && !memcmp(a->gap, b->gap, (size_t)(a->n - 1));
}

static void random_rhythm(rhythm_t *r, int level)
{
    memset(r, 0, sizeof(*r));
    if (level <= 1) { /* only how many beats: 2, 3 or 4, all quick */
        r->n = rng_range(2, 4);
        memset(r->gap, 'S', (size_t)(r->n - 1));
        return;
    }
    r->n = level == 2 ? 3 : 4;
    for (int i = 0; i < r->n - 1; i++)
        r->gap[i] = rng_range(0, 1) ? 'L' : 'S';
}

static void tamburo_setup(quiz_t *q, int level)
{
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    for (int i = 0; i < 3; i++) {
        bool dup;
        do {
            random_rhythm(&TB.opt[i], level);
            dup = false;
            for (int k = 0; k < i; k++)
                dup |= same_rhythm(&TB.opt[i], &TB.opt[k]);
        } while (dup);
    }
    TB.play = NULL;
    TB.shown = false;
    TB.replay = false;
    TB.beat_t = 99;
}

static void play(const rhythm_t *r)
{
    TB.play = r;
    TB.play_t = 0;
}

static void tamburo_tick(quiz_t *q)
{
    TB.beat_t++;
    if (TB.replay && !voice_busy()) {
        TB.replay = false;
        play(&TB.opt[q->correct]);
    }
    if (!TB.play)
        return;
    int t = 0;
    for (int i = 0; i < TB.play->n; i++) {
        if (TB.play_t == t) {
            sfx("clap");
            TB.beat_t = 0;
        }
        if (i < TB.play->n - 1)
            t += TB.play->gap[i] == 'L' ? GAP_L : GAP_S;
    }
    if (++TB.play_t > rhythm_len(TB.play))
        TB.play = NULL;
}

static bool tamburo_prepare(quiz_t *q)
{
    if (q->t == 1)
        say(q->duel ? story_line("dl_ritmo_ascolta") : "dl_ritmo_ascolta");
    if (!TB.shown) {
        if (q->t > 20 && !voice_busy()) {
            play(&TB.opt[q->correct]);
            TB.shown = true;
        }
        return false;
    }
    return !TB.play && TB.beat_t > 20;
}

static void tamburo_ask(quiz_t *q)
{
    say("dl_ritmo_quale");
    TB.replay = q->state == Q_INPUT; /* B, or asked again: the drum again, after the voice */
}

static void tamburo_hover(quiz_t *q, int i)
{
    TB.replay = false;
    play(&TB.opt[i]);
}

static bool tamburo_hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("dl_ritmo_ancora");
        q->hint_wait = 1;
        return false;
    }
    if (q->hint_step == 1) { /* after the voice: the right rhythm again */
        play(&TB.opt[q->correct]);
        q->hint_wait = rhythm_len(&TB.opt[q->correct]) + 20;
        return false;
    }
    tamburo_ask(q);
    return true;
}

static void tamburo_draw(quiz_t *q)
{
    /* the monster and its big drum, which jumps at every beat */
    const char *fname = foe_sprite(q, TB.beat_t < 8 ? 'r' : 'c');
    int cx = q->panel_cx, base = q->panel_cy + 50;
    /* 0.13: every beat squashes it and throws it up a little */
    int sq = TB.beat_t < 3 ? -4 : (TB.beat_t < 8 ? 3 : 0);
    actor_draw(fname, cx - 60, base - (TB.beat_t >= 3 && TB.beat_t < 8 ? 3 : 0), 1, 0, 0, sq, -sq);
    const sprite_t *d = gfx_sprite("tamburo");
    int k = TB.beat_t < 4 ? 1 : 0;
    gfx_blit_scaled(d, cx + 30 - d->w, base - 2 * d->h + k * 2, 2, 0);
    if (TB.beat_t == 0)
        fx_sparkles(cx + 30, base - 2 * d->h, 10, 4);
}

static void tamburo_card(quiz_t *q, int i, int cx, int cy, int style)
{
    /* one dot per beat: close together = quick, far apart = slow */
    const rhythm_t *r = &TB.opt[i];
    int units = 0;
    for (int k = 0; k < r->n - 1; k++)
        units += r->gap[k] == 'L' ? 2 : 1;
    int unit = units ? clampi(40 / units, 7, 10) : 0, x = cx - units * unit / 2;
    uint16_t c = style == CARD_OFF ? COL_GREY : (TB.play == r && TB.beat_t < 6 ? COL_HOT : COL_INK);
    for (int k = 0; k < r->n; k++) {
        dot(x, cy, c);
        if (k < r->n - 1)
            x += (r->gap[k] == 'L' ? 2 : 1) * unit;
    }
}

static void tamburo_value(quiz_t *q, int i, char *buf, size_t n)
{
    /* "3-SL": three beats, a quick gap then a slow one */
    const rhythm_t *r = &TB.opt[i];
    int l = snprintf(buf, n, "%d", r->n);
    if (r->n > 1 && l + r->n < (int)n) {
        buf[l++] = '-';
        for (int k = 0; k < r->n - 1; k++)
            buf[l++] = r->gap[k];
        buf[l] = 0;
    }
}

static const char *tamburo_subject(quiz_t *q)
{
    static char s[24], v[16];
    tamburo_value(q, q->correct, v, sizeof(v));
    snprintf(s, sizeof(s), "tamburo:%s", v);
    return s;
}

const quiz_def_t QUIZ_TAMBURO = {
    .name = "ritmo",
    .game = GAME_RITMO,
    .bg = "bg_palco",
    .card = QUIZ_CARD_PICTURE,
    .setup = tamburo_setup,
    .prepare = tamburo_prepare,
    .tick = tamburo_tick,
    .ask = tamburo_ask,
    .hover = tamburo_hover,
    .hint = tamburo_hint,
    .draw = tamburo_draw,
    .card_draw = tamburo_card,
    .card_value = tamburo_value,
    .subject = tamburo_subject,
};

/* ------------------------------------------------------------------ QUIZ_DOVE */

enum { W_SOPRA, W_SOTTO, W_SX, W_DX, W_DENTRO, W_FUORI };
static const char *const W_ID[6] = {"sopra", "sotto", "sinistra", "destra", "dentro", "fuori"};
static const char *const W_VOICE[6] = {"dv_w_sopra", "dv_w_sotto", "dv_w_sinistra", "dl_destra", "dv_w_dentro",
                                       "dv_w_fuori"};

static struct {
    bool box;      /* the box, else the table */
    int opt[3];
} DV;

static void dove_setup(quiz_t *q, int level)
{
    static const int TABLE_EASY[3] = {W_SOPRA, W_SOTTO, W_DX};
    static const int TABLE[4] = {W_SOPRA, W_SOTTO, W_SX, W_DX};
    static const int BOX[3] = {W_DENTRO, W_FUORI, W_SOPRA};
    DV.box = rng_range(0, 2) == 0;
    int pool[4], n = 3;
    if (DV.box)
        memcpy(pool, BOX, sizeof(BOX));
    else if (level <= 2)
        memcpy(pool, TABLE_EASY, sizeof(TABLE_EASY));
    else {
        memcpy(pool, TABLE, sizeof(TABLE));
        n = 4;
    }
    for (int i = n - 1; i > 0; i--) { /* shuffle, keep three */
        int j = rng_range(0, i), t = pool[i];
        pool[i] = pool[j];
        pool[j] = t;
    }
    if (!DV.box && level <= 2 && pool[0] == W_DX) { /* the easy levels ask sopra/sotto */
        pool[0] = pool[1];
        pool[1] = W_DX;
    }
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 1; i < 3; i++)
        DV.opt[i] = i == q->correct ? pool[0] : pool[k++];
}

static bool dove_prepare(quiz_t *q) { return q->t > 20; }

static void dove_ask(quiz_t *q)
{
    char id[40];
    snprintf(id, sizeof(id), "dl_dove_%s_%s", W_ID[DV.opt[q->correct]], DV.box ? "scatola" : "tavolo");
    say(id);
}

static void dove_hover(quiz_t *q, int i) { say(W_VOICE[DV.opt[i]]); }

static bool dove_hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say(W_VOICE[DV.opt[q->correct]]);
        return false;
    }
    dove_ask(q);
    return true;
}

static void dove_draw(quiz_t *q)
{
    /* the monster with the star in its hands, next to the table (the box) */
    const sprite_t *s = gfx_sprite(foe_sprite(q, 'c'));
    int base = q->panel_cy + 50;
    actor_draw(foe_sprite(q, 'c'), q->panel_cx - 88 + s->w / 2, base, 1, 0, 0, 0, 0);
    const sprite_t *st = gfx_sprite("star_on");
    gfx_blit(st, q->panel_cx - 88 + s->w / 2 - 8, base - s->h - 18 - swing((int)G.frame, 64, 2), 0);
    const sprite_t *o = gfx_sprite(DV.box ? "scatola" : "tavolo");
    gfx_blit_scaled(o, q->panel_cx + 36 - o->w, base - 2 * o->h, 2, 0);
}

static void mini(const char *name, int x, int y, int style)
{
    off_tint(style);
    gfx_blit(gfx_sprite(name), x, y, 0);
    gfx_set_tint(0, 0);
}

static void mini_star(int sx, int sy, int style)
{
    const sprite_t *st = gfx_sprite("lstar_on");
    if (style == CARD_OFF)
        gfx_blit_solid(st, sx - st->w / 2, sy - st->h / 2, COL_GREY, 0);
    else
        gfx_blit(st, sx - st->w / 2, sy - st->h / 2, 0);
}

static void dove_card(quiz_t *q, int i, int cx, int cy, int style)
{
    int w = DV.opt[i];
    if (!DV.box) { /* a little table (30x20): the star on it, under it, beside it */
        int sx = cx, sy = cy;
        switch (w) {
        case W_SOPRA: sx = cx, sy = cy - 12; break;
        case W_SOTTO: sx = cx, sy = cy + 9; break;
        case W_SX: sx = cx - 21, sy = cy + 9; break;
        default: sx = cx + 21, sy = cy + 9; break;
        }
        if (w == W_SOTTO) /* under: the table in front of it */
            mini_star(sx, sy, style);
        mini("mini_tavolo", cx - 15, cy - 6, style);
        if (w != W_SOTTO)
            mini_star(sx, sy, style);
        return;
    }
    /* a little box (26x17): the star inside (peeping out), outside, on the closed lid */
    int bx = cx - 13, by = cy - 1;
    switch (w) {
    case W_DENTRO:
        mini("mini_scatola", bx, by, style);
        mini_star(cx, cy + 1, style);
        mini("mini_scatola_fronte", bx, by, style); /* the front of the box over it */
        break;
    case W_FUORI:
        mini("mini_scatola", bx, by, style);
        mini_star(cx + 21, cy + 10, style);
        break;
    default:
        mini("mini_scatola_chiusa", bx, by, style);
        mini_star(cx, cy - 8, style);
        break;
    }
}

static void dove_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", W_ID[DV.opt[i]]); }

static const char *dove_subject(quiz_t *q)
{
    static char s[32];
    snprintf(s, sizeof(s), "dove:%s:%s", DV.box ? "scatola" : "tavolo", W_ID[DV.opt[q->correct]]);
    return s;
}

const quiz_def_t QUIZ_DOVE = {
    .name = "dove",
    .game = GAME_DOVE,
    .bg = "bg_palco",
    .card = QUIZ_CARD_PICTURE,
    .setup = dove_setup,
    .prepare = dove_prepare,
    .ask = dove_ask,
    .hover = dove_hover,
    .hint = dove_hint,
    .draw = dove_draw,
    .card_draw = dove_card,
    .card_value = dove_value,
    .subject = dove_subject,
};

/* ------------------------------------------------------------------ QUIZ_SPARITO */

#define MAX_ROW 4

static struct {
    int n, item[MAX_ROW];  /* the make-up in a row */
    int gone;              /* index in the row of the one that disappears */
    int opt[3];            /* items on the cards */
    int show_t;            /* frames of the row on show, -1 = not yet */
    int peek;              /* guided help: frames of the whole row shown again */
    bool poofed;
} SP;

static void sparito_setup(quiz_t *q, int level)
{
    SP.n = level <= 2 ? 3 : MAX_ROW;
    for (int i = 0; i < SP.n; i++) {
        bool dup;
        do {
            SP.item[i] = rng_range(0, ITEM_COUNT - 1);
            dup = false;
            for (int k = 0; k < i; k++)
                dup |= SP.item[k] == SP.item[i];
        } while (dup);
    }
    SP.gone = rng_range(0, SP.n - 1);
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    int others[MAX_ROW], no = 0;
    for (int i = 0; i < SP.n; i++)
        if (i != SP.gone)
            others[no++] = SP.item[i];
    for (int i = no - 1; i > 0; i--) {
        int j = rng_range(0, i), t = others[i];
        others[i] = others[j];
        others[j] = t;
    }
    for (int i = 0, k = 0; i < 3; i++)
        SP.opt[i] = i == q->correct ? SP.item[SP.gone] : others[k++];
    SP.show_t = -1;
    SP.peek = 0;
    SP.poofed = false;
}

static int row_x(const quiz_t *q, int i) { return q->panel_cx + 30 + (i * 2 - (SP.n - 1)) * 20; }

static bool sparito_prepare(quiz_t *q)
{
    int look = 200 - 20 * clampi(q->level, 1, 5); /* the higher, the shorter the look */
    if (q->t == 1)
        say("dl_sparito_guarda");
    if (SP.show_t < 0) {
        if (q->t > 20 && !voice_busy())
            SP.show_t = 0;
        return false;
    }
    if (++SP.show_t == look) { /* poof! */
        SP.poofed = true;
        sfx("poof");
        fx_sparkles(row_x(q, SP.gone), q->panel_cy - 4, 20, 20);
    }
    return SP.show_t > look + 30;
}

static void sparito_tick(quiz_t *q)
{
    if (SP.peek > 0)
        SP.peek--;
}

static void sparito_ask(quiz_t *q) { say("dl_sparito_quale"); }

static void sparito_hover(quiz_t *q, int i)
{
    char id[32];
    snprintf(id, sizeof(id), "t_%s", ITEMS[SP.opt[i]].id);
    say(id);
}

static bool sparito_hint(quiz_t *q)
{
    if (q->hint_step == 0) { /* look again at the whole row */
        say("dl_sparito_ancora");
        SP.peek = 120;
        q->hint_wait = 120;
        return false;
    }
    sparito_ask(q);
    return true;
}

static void sparito_draw(quiz_t *q)
{
    actor_draw(foe_sprite(q, 'c'), q->panel_cx - 78, q->panel_cy + 54, 1, 0, 0, 0, 0);
    if (SP.show_t < 0)
        return;
    char name[32];
    for (int i = 0; i < SP.n; i++) {
        if (i == SP.gone && SP.poofed && SP.peek == 0) { /* an empty place */
            gfx_round_rect(row_x(q, i) - 18, q->panel_cy - 22, 36, 36, COL_LILAC_L, COL_LILAC);
            continue;
        }
        snprintf(name, sizeof(name), "rw_%s", ITEMS[SP.item[i]].id);
        const sprite_t *it = gfx_sprite(name);
        gfx_blit(it, row_x(q, i) - it->w / 2, q->panel_cy - 4 - it->h / 2, 0);
    }
}

static void sparito_card(quiz_t *q, int i, int cx, int cy, int style)
{
    char name[32];
    snprintf(name, sizeof(name), "rw_%s", ITEMS[SP.opt[i]].id);
    const sprite_t *it = gfx_sprite(name);
    off_tint(style);
    gfx_blit(it, cx - it->w / 2, cy - it->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void sparito_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", ITEMS[SP.opt[i]].id); }

static const char *sparito_subject(quiz_t *q)
{
    static char s[40];
    snprintf(s, sizeof(s), "sparito:%s", ITEMS[SP.item[SP.gone]].id);
    return s;
}

const quiz_def_t QUIZ_SPARITO = {
    .name = "memory",
    .game = GAME_MEMORY,
    .bg = "bg_camerino",
    .card = QUIZ_CARD_PICTURE,
    .setup = sparito_setup,
    .prepare = sparito_prepare,
    .tick = sparito_tick,
    .ask = sparito_ask,
    .hover = sparito_hover,
    .hint = sparito_hint,
    .draw = sparito_draw,
    .card_draw = sparito_card,
    .card_value = sparito_value,
    .subject = sparito_subject,
};
