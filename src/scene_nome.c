/* Deva's Awesome Adventures - "Il mio nome": she writes her name, then short
 * words with a picture, choosing each letter out of three cards.
 *
 * Levels: 1 her name with the model letters shown faintly in the boxes; 2
 * short words (UVA, MELA...), model shown; 3 longer words and look-alike
 * letters (E/F, M/N, P/R...); 4 no model, the voice says the letter names;
 * 5 no model and look-alike letters. Every word is a multi-step question of
 * the quiz engine: one letter per step, one star per word.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

#define MAX_LEN 12
#define SLOT 30
#define SLOT_STEP 34

static const char *SHORT[] = {"UVA", "APE", "MELA", "LUNA", "SOLE", "CASA", "UOVO"};
static const char *LONG[] = {"ISOLA", "PALLA", "TORTA", "FIORE", "CUORE", "PESCE", "MELA", "LUNA", "SOLE", "CASA"};
static const char *LONGEST[] = {"STELLA", "ISOLA", "PALLA", "TORTA", "FIORE", "CUORE", "PESCE"};

/* the 21 letters of the Italian alphabet (distractors come from here) and the
 * upper-case ones easily taken for one another */
static const char ALPHABET[] = "ABCDEFGHILMNOPQRSTUVZ";
static const char *LOOKALIKE[] = {"MN", "EF", "PRB", "OQCGD", "ILT", "UV", "SZ"};

static struct {
    char word[MAX_LEN + 1];
    bool is_name;
    int len, pos;           /* letters written so far = pos */
    bool model;             /* faint model letters in the boxes */
    bool model_now;         /* model of the current box shown by the guided help */
    bool similar;           /* look-alike distractors */
    char opt[3];
    int last_word;          /* index into the level's list, to avoid repeats */
    hero_t portrait;        /* Deva waving next to her name */
} N;

static const char *lookalike_group(char c)
{
    for (int i = 0; i < ARRAY_LEN(LOOKALIKE); i++)
        if (strchr(LOOKALIKE[i], c))
            return LOOKALIKE[i];
    return "";
}

static void say_letter(char c, bool queue)
{
    char id[8];
    snprintf(id, sizeof(id), "lt_%c", c - 'A' + 'a');
    if (queue)
        say_then(id);
    else
        say(id);
}

static void make_cards(quiz_t *q)
{
    char target = N.word[N.pos];
    const char *group = lookalike_group(target);
    char d[2];
    for (int k = 0; k < 2; k++) {
        for (int tries = 0;; tries++) {
            char c;
            if (N.similar && k == 0 && strlen(group) > 1 && tries < 50)
                c = group[rng_range(0, (int)strlen(group) - 1)];
            else
                c = ALPHABET[rng_range(0, (int)sizeof(ALPHABET) - 2)];
            bool bad = c == target || (k == 1 && c == d[0]);
            /* the second distractor, and both at easy levels, must not look alike */
            if (!bad && (!N.similar || k == 1) && strchr(group, c) && tries < 200)
                bad = true;
            if (!bad) {
                d[k] = c;
                break;
            }
        }
    }
    q->correct = rng_range(0, 2);
    int k = 0;
    for (int i = 0; i < 3; i++)
        N.opt[i] = i == q->correct ? target : d[k++];
    N.model_now = false;
}

static void pick_word(quiz_t *q, int level)
{
    bool name = level == 1 || (q->question == 0 && (level <= 3 || rng_range(0, 1)));
    N.is_name = name;
    if (name) {
        snprintf(N.word, sizeof(N.word), "%.12s", child_name());
        return;
    }
    const char *const *list = SHORT;
    int n = ARRAY_LEN(SHORT);
    if (level == 3 || level == 4) {
        list = LONG;
        n = ARRAY_LEN(LONG);
    } else if (level >= 5) {
        list = LONGEST;
        n = ARRAY_LEN(LONGEST);
    }
    int w;
    do
        w = rng_range(0, n - 1);
    while (n > 1 && w == N.last_word);
    N.last_word = w;
    snprintf(N.word, sizeof(N.word), "%s", list[w]);
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        N.last_word = -1;
    pick_word(q, level);
    N.len = (int)strlen(N.word);
    N.pos = 0;
    N.model = level <= 3;
    N.similar = level == 3 || level == 5;
    q->steps = N.len;
    make_cards(q);
    /* a long name goes on two rows: Deva steps up to make room */
    hero_init(&N.portrait, q->panel_cx, q->panel_cy + (N.len > 6 ? -4 : 6));
    hero_play(&N.portrait, HA_WAVE, 0);
}

static void tick(quiz_t *q) { hero_update(&N.portrait); }

static void next_step(quiz_t *q)
{
    N.pos++;
    make_cards(q);
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("pop");
    return q->t > 20;
}

static void say_word(bool queue)
{
    char id[32];
    snprintf(id, sizeof(id), "w_%s", N.word);
    for (char *c = id + 2; *c; c++)
        *c = (char)(*c - 'A' + 'a');
    if (queue)
        say_then(id);
    else
        say(id);
}

static void ask(quiz_t *q)
{
    if (N.pos == 0) {
        if (N.is_name) {
            say("nome_tuo");
        } else {
            say("nome_parola");
            say_word(true);
        }
        if (N.model && q->question == 0)
            say_then("nome_trova");
        if (!N.model)
            say_letter(N.word[0], true);
    } else if (!N.model || q->state == Q_INPUT) {
        /* B (or a long silence) in the middle of the word: the letter to find */
        say_letter(N.word[N.pos], false);
    }
}

static void hover(quiz_t *q, int i) { say_letter(N.opt[i], false); }

static void right(quiz_t *q)
{
    if (N.is_name)
        say_then(name_voice("nome_fatto_nome") ? "nome_fatto_nome" : "nome_fatto_tuo");
    else {
        say_then("nome_fatto");
        say_word(true);
    }
}

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        N.model_now = true; /* the model letter appears in the box */
        say_letter(N.word[N.pos], false);
        return false;
    }
    say("nome_scegli");
    return true;
}

/* 0.12: on the sign of light bulbs (bg_insegna) a short word gets big tiles, its letters twice as
   big; the duels keep the small boxes */
#define BIG_W 40
#define BIG_H 48
#define BIG_STEP 46

static void draw(quiz_t *q)
{
    bool sign = !q->duel, big = sign && N.len <= 4;
    int cx = q->panel_cx, pic_cy = q->panel_cy - (sign ? 26 : 30);
    /* the picture of the word, or Deva for the name */
    if (N.is_name) {
        N.portrait.talking = false;
        hero_draw(&N.portrait, &G.prog);
    } else {
        char name[32];
        snprintf(name, sizeof(name), "pic_%s", N.word);
        for (char *c = name + 4; *c; c++)
            *c = (char)(*c - 'A' + 'a');
        const sprite_t *s = gfx_sprite(name);
        int k = q->state == Q_PREPARE && q->t < 6 ? 1 : 2;
        gfx_blit_scaled(s, cx - s->w * k / 2, pic_cy - s->h * k / 2, k, 0);
    }
    /* the boxes: written letters, the current box glowing, faint models;
       a name longer than 6 letters goes on two rows */
    int rows = N.len > 6 ? 2 : 1, per = (N.len + rows - 1) / rows;
    int w = big ? BIG_W : SLOT, h = big ? BIG_H : SLOT, step = big ? BIG_STEP : SLOT_STEP, k = big ? 2 : 1;
    bool done = q->state == Q_RIGHT;
    for (int i = 0; i < N.len; i++) {
        int r = i / per, n = r == 0 ? per : N.len - per;
        int x = cx - (n * step - (step - w)) / 2 + (i % per) * step;
        int y = big ? q->panel_cy + 13 : (rows == 1 ? q->panel_cy + 30 : q->panel_cy + 2 + r * SLOT_STEP), dy = 0;
        bool written = i < N.pos || done || (q->state == Q_STEP && i == N.pos);
        if (done && q->t < 90)
            dy = ((q->t / 4 + N.len - i) % 8) < 2 ? -3 : 0; /* happy wave */
        if (i == N.pos && !done && q->state != Q_STEP)
            gfx_blit(gfx_sprite(big ? "lettera_box_sel" : "scard_sel"), x - 2, y - 2, 0);
        else
            gfx_blit(gfx_sprite(big ? "lettera_box" : "scard"), x, y + dy, 0);
        char s[2] = {N.word[i], 0};
        if (written) {
            gfx_text_big(s, x + w / 2, y + h / 2 + dy, i == N.pos && !done ? NUM_SELECTED : NUM_NORMAL, k);
        } else if (N.model || (i == N.pos && N.model_now)) {
            gfx_set_tint(rgb565(0xff, 0xf4, 0xfa), 170); /* faint: a model to copy */
            gfx_text_big(s, x + w / 2, y + h / 2, NUM_NORMAL, k);
            gfx_set_tint(0, 0);
        }
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    char s[2] = {N.opt[i], 0};
    gfx_text_big(s, cx, cy, style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL), 2);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%c", N.opt[i]); }

static const char *subject(quiz_t *q)
{
    static char s[32];
    snprintf(s, sizeof(s), "%s:%s", N.is_name ? "nome" : "parola", N.word);
    return s;
}

const quiz_def_t QUIZ_NOME = {
    .name = "nome",
    .game = GAME_NOME,
    .bg = "bg_insegna", /* 0.12: the stage, her name on a sign of light bulbs */
    .intro = "nome_intro",
    .card = QUIZ_CARD_NUMBER,
    .max_questions = 3,
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

static void enter(void) { quiz_enter(&QUIZ_NOME); }

const scene_t SCENE_NOME = {enter, quiz_update, quiz_draw};
