/* Deva's Awesome Adventures - "Le emozioni di Deva": how does Deva feel?
 * Three cards with little faces (felice, triste, arrabbiata, spaventata,
 * sorpresa); the question is either Deva's own face, big in the panel, or a
 * little story ("il palloncino è volato via...") with its picture.
 *
 * Levels: 1 her face, three feelings; 2 her face, all five; 3 easy stories;
 * 4 all the stories; 5 both. A story never offers as a wrong card a feeling
 * that would fit it too (a torn drawing: angry, but sad is fair as well).
 * Guided help: a sentence on how that feeling shows ("quando siamo tristi
 * piangiamo...") and Deva shows it on her face.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

enum { E_FELICE, E_TRISTE, E_ARRABBIATA, E_SPAVENTATA, E_SORPRESA, E_COUNT };
static const char *EMO_IDS[E_COUNT] = {"felice", "triste", "arrabbiata", "spaventata", "sorpresa"};
static const hero_expr_t EXPR[E_COUNT] = {EXPR_FELICE, EXPR_TRISTE, EXPR_ARRABBIATA, EXPR_SPAVENTATA,
                                          EXPR_SORPRESA};

typedef struct {
    const char *id;  /* voice emo_s_<id>, picture pic_<id> */
    int emo, also;   /* the feeling, another one that fits too (-1 none) */
    bool easy;
} story_t;

static const story_t STORIES[] = {
    {"torta", E_FELICE, E_SORPRESA, true},          {"corona", E_FELICE, E_SORPRESA, false},
    {"microfono", E_FELICE, -1, true},              {"palloncino", E_TRISTE, E_ARRABBIATA, true},
    {"gelato_caduto", E_TRISTE, E_ARRABBIATA, true}, {"disegno", E_ARRABBIATA, E_TRISTE, false},
    {"letto", E_ARRABBIATA, E_TRISTE, false},       {"tuono", E_SPAVENTATA, E_SORPRESA, true},
    {"luna", E_SPAVENTATA, E_SORPRESA, false},      {"regalo", E_SORPRESA, E_FELICE, true},
    {"pulcino", E_SORPRESA, E_FELICE, false},
};

enum { M_FACCIA, M_STORIA };

static struct {
    int mode;
    int emo;           /* right feeling */
    int story;         /* M_STORIA */
    int opt[3];        /* feelings on the cards */
    bool show_face;    /* M_STORIA: Deva's face next to the picture (help, or solved) */
    int last_story, last_emo;
    hero_t face;       /* the big Deva in the panel */
} E;

static void set_face(int emo)
{
    E.face.expr = emo >= 0 ? EXPR[emo] : EXPR_NONE;
}

/* the duel of the tale asks, just before "how does the witch feel?", the same
   feeling on Deva: "faccia:triste" (her face), "storia:luna" (a little story) */
static const char *s_force;
void emozioni_force(const char *what) { s_force = what; }

static bool forced(int *mode, int *emo, int *story)
{
    if (!s_force)
        return false;
    const char *what = s_force, *arg = strchr(what, ':');
    s_force = NULL;
    if (!arg)
        return false;
    arg++;
    if (!strncmp(what, "faccia:", 7)) {
        for (int e = 0; e < E_COUNT; e++)
            if (!strcmp(arg, EMO_IDS[e])) {
                *mode = M_FACCIA, *emo = e;
                return true;
            }
    } else {
        for (int s = 0; s < ARRAY_LEN(STORIES); s++)
            if (!strcmp(arg, STORIES[s].id)) {
                *mode = M_STORIA, *story = s, *emo = STORIES[s].emo;
                return true;
            }
    }
    return false;
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        E.last_story = E.last_emo = -1;
    int fmode, femo = 0, fstory = 0;
    bool force = forced(&fmode, &femo, &fstory);
    E.mode = force ? fmode : level <= 2 ? M_FACCIA : (level <= 4 ? M_STORIA : (rng_range(0, 1) ? M_STORIA : M_FACCIA));
    bool excluded[E_COUNT] = {false};
    if (force && E.mode == M_FACCIA) {
        E.emo = femo;
        for (int e = 3; e < E_COUNT && level == 1; e++) /* level 1: the three feelings she knows */
            excluded[e] = true;
    } else if (force) {
        E.story = E.last_story = fstory;
        E.emo = femo;
        if (STORIES[fstory].also >= 0)
            excluded[STORIES[fstory].also] = true;
    } else if (E.mode == M_FACCIA) {
        int pool = level == 1 ? 3 : E_COUNT; /* level 1: happy, sad, angry */
        do
            E.emo = rng_range(0, pool - 1);
        while (E.emo == E.last_emo);
        for (int e = pool; e < E_COUNT; e++)
            excluded[e] = true;
    } else {
        int n = ARRAY_LEN(STORIES), s;
        do
            s = rng_range(0, n - 1);
        while (s == E.last_story || (level == 3 && !STORIES[s].easy));
        E.story = E.last_story = s;
        E.emo = STORIES[s].emo;
        if (STORIES[s].also >= 0)
            excluded[STORIES[s].also] = true;
    }
    E.last_emo = E.emo;
    excluded[E.emo] = true;
    if (E.mode == M_FACCIA && level <= 3) { /* scared and surprised faces side by side: from level 4 */
        if (E.emo == E_SPAVENTATA)
            excluded[E_SORPRESA] = true;
        if (E.emo == E_SORPRESA)
            excluded[E_SPAVENTATA] = true;
    }
    q->gentle = E.emo == E_TRISTE || E.emo == E_ARRABBIATA || E.emo == E_SPAVENTATA;
    /* two other feelings among the allowed ones */
    int others[E_COUNT], no = 0;
    for (int e = 0; e < E_COUNT; e++)
        if (!excluded[e])
            others[no++] = e;
    for (int i = no - 1; i > 0; i--) {
        int j = rng_range(0, i), t = others[i];
        others[i] = others[j];
        others[j] = t;
    }
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        E.opt[i] = i == q->correct ? E.emo : others[k++];
    q->card = QUIZ_CARD_PICTURE;
    E.show_face = false;
    int fx = E.mode == M_FACCIA ? q->panel_cx : q->panel_cx + 50;
    hero_init(&E.face, fx, q->panel_cy + 64);
    set_face(E.mode == M_FACCIA ? E.emo : -1);
}

static void tick(quiz_t *q)
{
    hero_update(&E.face);
    if (E.mode == M_STORIA && (q->state == Q_RIGHT) && !E.show_face) {
        E.show_face = true; /* solved: Deva shows how she feels */
        set_face(E.emo);
    }
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("pop");
    return q->t > 20;
}

static void say_emo(const char *prefix, int emo, bool queue)
{
    char id[40];
    snprintf(id, sizeof(id), "%s%s", prefix, EMO_IDS[emo]);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void ask(quiz_t *q)
{
    if (E.mode == M_STORIA) {
        char id[40];
        snprintf(id, sizeof(id), "emo_s_%s", STORIES[E.story].id);
        say(id);
        say_then("emo_come");
    } else {
        say("emo_come");
    }
}

static void hover(quiz_t *q, int i) { say_emo("emo_", E.opt[i], false); }

static void right(quiz_t *q) { say_emo("emo_e_", E.emo, true); }

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        /* how this feeling shows, and Deva shows it */
        E.show_face = true;
        set_face(E.emo);
        E.face.t = 0;
        say_emo("emo_d_", E.emo, false);
        return false;
    }
    say("emo_scegli");
    return true;
}

static void draw(quiz_t *q)
{
    int cx = q->panel_cx, cy = q->panel_cy;
    if (E.mode == M_FACCIA) {
        E.face.talking = false;
        hero_draw_zoom(&E.face, &G.prog, 2);
        return;
    }
    char name[40];
    snprintf(name, sizeof(name), "pic_%s", STORIES[E.story].id);
    const sprite_t *s = gfx_sprite(name);
    int k = q->state == Q_PREPARE && q->t < 6 ? 1 : 2;
    int px = E.show_face ? cx - 50 : cx;
    gfx_blit_scaled(s, px - s->w * k / 2, cy - 8 - s->h * k / 2, k, 0);
    if (E.show_face) {
        E.face.talking = false;
        hero_draw_zoom(&E.face, &G.prog, 2);
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    char name[32];
    snprintf(name, sizeof(name), "emo_%s", EMO_IDS[E.opt[i]]);
    const sprite_t *s = gfx_sprite(name);
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", EMO_IDS[E.opt[i]]); }

static const char *subject(quiz_t *q)
{
    static char s[40];
    if (E.mode == M_FACCIA)
        snprintf(s, sizeof(s), "faccia:%s", EMO_IDS[E.emo]);
    else
        snprintf(s, sizeof(s), "storia:%s", STORIES[E.story].id);
    return s;
}

const quiz_def_t QUIZ_EMOZIONI = {
    .name = "emozioni",
    .game = GAME_EMOZIONI,
    .bg = "bg_salotto", /* 0.12: the sitting room, her face in the golden frame */
    .intro = "emo_intro",
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

static void enter(void) { quiz_enter(&QUIZ_EMOZIONI); }

const scene_t SCENE_EMOZIONI = {enter, quiz_update, quiz_draw};
