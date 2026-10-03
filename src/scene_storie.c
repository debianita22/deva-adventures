/* Deva's Awesome Adventures - "La storia in ordine": three little pictures of
 * a story (a seed that becomes a flower, an egg that hatches, an ice cream
 * that is eaten...) go into three numbered boxes, first to last.
 *
 * Levels: 1 the first picture is in its box, which one comes next? (the
 *   others are from different stories); 2 the three pictures of an easy
 *   story: "what comes first?", "and then?" - the last one goes by itself;
 *   3 the same with every story; 4 backwards: "how does it end?", "and at
 *   the beginning?"; 5 forwards or backwards. Each question is a multi-step
 *   question of the quiz engine; the story is told when it is complete.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

#define SLOT 44
#define SLOT_STEP 60
/* 0.12: the story is told in a picture book that unfolds in three pages (bg_libro): its pictures
   at 2x, 80 px apart, the middle of the pages at y 86 (tools/art/games6.py, BOOK); the duels keep
   the little boxes */
#define PAGE_STEP 80
#define PAGE_CY 86
#define PICT 72
#define EASY 4 /* the first stories are the easy ones */

static const char *STORY_IDS[] = {"seme", "uovo", "gelato", "palloncino", "candela", "giorno"};
enum { N_STORIES = 6 };

static struct {
    int story;
    int card_story[3], card_frame[3]; /* frame 1..3 of a story on each card */
    int slot[3];                      /* frame shown in each box, 0 = empty */
    bool backward, next_only;
    int want;                         /* frame asked in this step */
    int last_story;
    unsigned told;                    /* stories of this round (no repeats while others are left) */
    int fill_t;                       /* frame counter of the box animation */
} S;

static int want_of_step(int step)
{
    if (S.next_only)
        return 2;
    if (S.backward)
        return step == 0 ? 3 : 1;
    return step == 0 ? 1 : 2;
}

static int card_with(int story, int frame)
{
    for (int i = 0; i < 3; i++)
        if (S.card_story[i] == story && S.card_frame[i] == frame)
            return i;
    return 0;
}

static bool used(int i) { return S.card_story[i] == S.story && S.slot[S.card_frame[i] - 1] != 0; }

static void setup(quiz_t *q, int level)
{
    if (q->question == 0) {
        S.last_story = -1;
        S.told = 0;
    }
    int pool = level <= 2 ? EASY : N_STORIES;
    unsigned all = (1u << pool) - 1;
    if ((S.told & all) == all) /* every story of the level told: start again (not the last one) */
        S.told = 0;
    do
        S.story = rng_range(0, pool - 1);
    while (S.story == S.last_story || ((S.told >> S.story) & 1u));
    S.last_story = S.story;
    S.told |= 1u << S.story;
    memset(S.slot, 0, sizeof(S.slot));
    S.next_only = level == 1;
    S.backward = level == 4 || (level == 5 && rng_range(0, 1));
    q->card = QUIZ_CARD_PICTURE;
    if (S.next_only) {
        /* the beginning is shown; the cards: what comes next, and two pictures of other stories */
        S.slot[0] = 1;
        int o1, o2;
        do
            o1 = rng_range(0, N_STORIES - 1);
        while (o1 == S.story);
        do
            o2 = rng_range(0, N_STORIES - 1);
        while (o2 == S.story || o2 == o1);
        int c = rng_range(0, 2), k = 0;
        int v1, v2;
        rng_pair(1, 3, 1, 3, &v1, &v2);
        int others[2][2] = {{o1, v1}, {o2, v2}};
        for (int i = 0; i < 3; i++) {
            if (i == c) {
                S.card_story[i] = S.story;
                S.card_frame[i] = 2;
            } else {
                S.card_story[i] = others[k][0];
                S.card_frame[i] = others[k][1];
                k++;
            }
        }
        q->steps = 1;
    } else {
        int order[3] = {1, 2, 3};
        for (int i = 2; i > 0; i--) {
            int j = rng_range(0, i), t = order[i];
            order[i] = order[j];
            order[j] = t;
        }
        if (order[0] == 1 && order[1] == 2) { /* never already in order */
            order[1] = 3;
            order[2] = 2;
        }
        for (int i = 0; i < 3; i++) {
            S.card_story[i] = S.story;
            S.card_frame[i] = order[i];
        }
        q->steps = 2;
    }
    S.want = want_of_step(0);
    q->correct = card_with(S.story, S.want);
    S.fill_t = 0;
}

static int slot_cx(quiz_t *q, int i) { return q->panel_cx + (i - 1) * (q->duel ? SLOT_STEP : PAGE_STEP); }
static int slot_cy(quiz_t *q) { return q->duel ? q->panel_cy - 6 : PAGE_CY; }

static void tick(quiz_t *q)
{
    S.fill_t++;
    if ((q->state == Q_STEP || q->state == Q_RIGHT) && !S.slot[S.want - 1]) {
        S.slot[S.want - 1] = S.want; /* the chosen picture goes into its box */
        S.fill_t = 0;
        fx_sparkles(slot_cx(q, S.want - 1), slot_cy(q), 12, 8);
    }
    if (q->state == Q_RIGHT && q->t == 24) {
        for (int f = 1; f <= 3; f++) /* the last one goes by itself */
            if (!S.slot[f - 1]) {
                S.slot[f - 1] = f;
                sfx("pop");
                fx_sparkles(slot_cx(q, f - 1), slot_cy(q), 12, 8);
            }
    }
}

static void next_step(quiz_t *q)
{
    S.want = want_of_step(q->step);
    for (int i = 0; i < 3; i++)
        if (used(i))
            q->off[i] = true;
    q->correct = card_with(S.story, S.want);
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("pop");
    return q->t > 20;
}

static void ask(quiz_t *q)
{
    const char *id;
    if (S.next_only)
        id = "storie_dopo";
    else if (S.backward)
        id = q->step == 0 ? "storie_fine" : "storie_inizio";
    else
        id = q->step == 0 ? "storie_prima" : "storie_poi";
    say(id);
}

static void say_frame(int story, int frame, bool queue)
{
    char id[40];
    snprintf(id, sizeof(id), "storia_%s_%d", STORY_IDS[story], frame);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void hover(quiz_t *q, int i) { say_frame(S.card_story[i], S.card_frame[i], false); }

static void right(quiz_t *q)
{
    char id[40];
    snprintf(id, sizeof(id), "storia_%s", STORY_IDS[S.story]);
    say_then(id); /* the whole story, in order */
}

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say_frame(S.story, S.want, false);
        return false;
    }
    say("par_scegli");
    return true;
}

static void draw_frame(int story, int frame, int cx, int cy, int k)
{
    char name[40];
    snprintf(name, sizeof(name), "st_%s_%d", STORY_IDS[story], frame);
    const sprite_t *s = gfx_sprite(name);
    gfx_blit_scaled(s, cx - s->w * k / 2, cy - s->h * k / 2, k, 0);
}

/* the place of a picture still to come: a dashed frame on the page */
static void dashed_box(int x, int y, int w, int h, uint16_t c)
{
    for (int i = 0; i < w; i += 6) {
        gfx_fill_rect(x + i, y, imin(3, w - i), 2, c);
        gfx_fill_rect(x + i, y + h - 2, imin(3, w - i), 2, c);
    }
    for (int j = 0; j < h; j += 6) {
        gfx_fill_rect(x, y + j, 2, imin(3, h - j), c);
        gfx_fill_rect(x + w - 2, y + j, 2, imin(3, h - j), c);
    }
}

static void draw_book(quiz_t *q)
{
    int cy = slot_cy(q);
    bool done = q->state == Q_RIGHT && q->t > 24;
    for (int i = 0; i < 3; i++) {
        int cx = slot_cx(q, i), dy = 0;
        if (done && q->t < 110)
            dy = ((q->t / 4 + 3 - i) % 8) < 2 ? -3 : 0; /* happy wave */
        bool asked = !S.slot[i] && i == S.want - 1 && q->state != Q_PREPARE;
        if (S.slot[i]) {
            draw_frame(S.story, S.slot[i], cx, cy + dy, 2);
        } else {
            dashed_box(cx - PICT / 2, cy - PICT / 2, PICT, PICT,
                       asked ? rgb565(0xff, 0x5a, 0x9a) : rgb565(0xe0, 0xc8, 0xa8));
            const sprite_t *qm = gfx_sprite("qmark");
            if (asked && ((G.frame / 20) & 1))
                gfx_blit_scaled(qm, cx - qm->w, cy - qm->h, 2, 0);
        }
        /* the number of the page under its picture */
        gfx_number_small(i + 1, cx, cy + PICT / 2 + 12 + dy);
    }
}

static void draw(quiz_t *q)
{
    if (!q->duel) {
        draw_book(q);
        return;
    }
    int cy = slot_cy(q);
    bool done = q->state == Q_RIGHT && q->t > 24;
    for (int i = 0; i < 3; i++) {
        int cx = slot_cx(q, i), dy = 0;
        if (done && q->t < 110)
            dy = ((q->t / 4 + 3 - i) % 8) < 2 ? -3 : 0; /* happy wave */
        bool asked = !S.slot[i] && i == S.want - 1 && q->state != Q_PREPARE;
        gfx_blit(gfx_sprite(asked ? "story_slot_q" : "story_slot"), cx - SLOT / 2, cy - SLOT / 2 + dy, 0);
        if (S.slot[i]) {
            draw_frame(S.story, S.slot[i], cx, cy + dy, 1);
        } else {
            const sprite_t *qm = gfx_sprite("qmark");
            if (asked && ((G.frame / 20) & 1))
                gfx_blit(qm, cx - qm->w / 2, cy - qm->h / 2, 0);
        }
        /* the number of the box under it */
        gfx_number_small(i + 1, cx, cy + SLOT / 2 + 9 + dy);
    }
    /* little arrows between the boxes: the story goes this way */
    const sprite_t *ar = gfx_sprite("arrow_right");
    for (int i = 0; i < 2; i++) {
        int ax = (slot_cx(q, i) + slot_cx(q, i + 1)) / 2;
        gfx_set_tint(rgb565(0xff, 0xf4, 0xfa), 110);
        gfx_blit(ar, ax - ar->w / 2 + 1, cy - ar->h / 2, 0);
        gfx_set_tint(0, 0);
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (used(i))
        return; /* its picture is in a box now */
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    draw_frame(S.card_story[i], S.card_frame[i], cx, cy, 1);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    snprintf(buf, n, "%s%d", STORY_IDS[S.card_story[i]], S.card_frame[i]);
}

static const char *subject(quiz_t *q)
{
    static char s[40];
    snprintf(s, sizeof(s), "%s:%s", S.next_only ? "dopo" : (S.backward ? "indietro" : "avanti"), STORY_IDS[S.story]);
    return s;
}

const quiz_def_t QUIZ_STORIE = {
    .name = "storie",
    .game = GAME_STORIE,
    .bg = "bg_libro", /* 0.12: a bedtime picture book, three pages */
    .intro = "storie_intro",
    .card = QUIZ_CARD_PICTURE,
    .max_questions = 4,
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

static void enter(void) { quiz_enter(&QUIZ_STORIE); }

const scene_t SCENE_STORIE = {enter, quiz_update, quiz_draw};
