/* Deva's Awesome Adventures - "Trucca i mostri": the friends of the tale come
 * to the make-up salon. A client sits in front of the big mirror and says
 * what it wants ("Vuole l'ombretto viola! E il rossetto rosso!"); she picks
 * each item among three cards, and the make-up appears on the client's face.
 *
 * It trains colour and make-up words, listening, and remembering two or three
 * things at once. Levels: 1 one item, the wish drawn in a speech bubble, the
 * other cards of other kinds; 2 one item, the bubble shows only its shape (the
 * colour must be heard), the other cards same kind; 3 two items; 4 two items
 * among lookalikes, reminded only on request; 5 three items, no bubble.
 * Up to level 4 the bubble shows the shape of the item of this step.
 * At every step the cards hold exactly one of the wished items still missing,
 * so the order does not matter. Guided help: the wish again, and the right
 * card lights up.
 *
 * In the duels of the tale the client is the monster itself: make-up as a
 * spell ("mi fai il solletico!").
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
#include "story.h"
#include "ui.h"

typedef struct {
    const char *sprite;
    const char *id;     /* voices tm_ciao_<id>, tm_grazie_<id> */
    int eyes;           /* 1 = one big eye */
    int eye[2][2];      /* the eyeshadow: centre of its lower edge */
    int mouth[2];       /* centre of the lips */
    bool small;         /* witch and wizard: small lips */
    int cheek[2][2];    /* blush; the sticker goes above the second one */
    int top[2];         /* the crown: centre of its lower edge */
    int flower[2];
} client_t;

/* anchors measured on the drawings of tools/art/tale.py */
static const client_t CLIENTS[] = {
    {"mo_ciuffone_b", "ciuffone", 2, {{24, 25}, {40, 25}}, {32, 43}, false, {{18, 38}, {46, 38}}, {32, 17}, {24, 14}},
    {"mo_melmoso_b", "melmoso", 1, {{32, 21}, {32, 21}}, {32, 45}, false, {{18, 40}, {46, 40}}, {37, 13}, {21, 14}},
    {"mo_rocciolo_b", "rocciolo", 2, {{24, 27}, {40, 27}}, {32, 43}, false, {{17, 39}, {47, 39}}, {32, 18}, {46, 22}},
    {"mo_tuonello_b", "tuonello", 2, {{24, 28}, {40, 28}}, {32, 37}, false, {{18, 35}, {46, 35}}, {32, 10}, {44, 14}},
    {"strega_b", "strega", 2, {{21, 28}, {31, 28}}, {26, 38}, true, {{19, 35}, {33, 35}}, {26, 18}, {33, 19}},
    {"mago", "mago", 2, {{21, 26}, {31, 26}}, {26, 40}, true, {{19, 33}, {33, 33}}, {26, 22}, {33, 21}},
};
#define NCLIENTS ((int)(sizeof(CLIENTS) / sizeof(CLIENTS[0])))

/* the friends of the second adventure come to the salon once they are free */
static const client_t FRIENDS2[CH_COUNT] = {
    {"mo_polpone_b", "polpone", 2, {{24, 19}, {40, 19}}, {32, 36}, false, {{18, 32}, {46, 32}}, {32, 8}, {22, 10}},
    {"mo_lumacone_b", "lumacone", 2, {{7, 9}, {22, 9}}, {15, 43}, true, {{9, 42}, {22, 42}}, {15, 30}, {36, 20}},
    {"mo_nevone_b", "nevone", 2, {{27, 23}, {37, 23}}, {32, 34}, false, {{22, 33}, {42, 33}}, {32, 12}, {40, 12}},
    {"mo_fumino_b", "fumino", 2, {{25, 15}, {39, 15}}, {32, 33}, false, {{20, 27}, {44, 27}}, {32, 11}, {42, 10}},
    {"stregone_b", "stregone", 2, {{22, 26}, {30, 26}}, {26, 37}, true, {{20, 33}, {32, 33}}, {26, 17}, {33, 18}},
};

/* ...and the friends of the third (0.10.0; anchors measured on tools/art/tale3.py) */
static const client_t FRIENDS3[CH_COUNT] = {
    {"mo_caramellone_b", "caramellone", 2, {{25, 17}, {39, 17}}, {32, 30}, false, {{19, 29}, {45, 29}}, {32, 8}, {45, 10}},
    {"mo_fungone_b", "fungone", 2, {{26, 33}, {38, 33}}, {32, 46}, false, {{22, 42}, {42, 42}}, {32, 6}, {46, 12}},
    {"mo_ranocchione_b", "ranocchione", 2, {{20, 16}, {44, 16}}, {32, 38}, false, {{17, 36}, {47, 36}}, {32, 21}, {50, 16}},
    {"mo_trombone_b", "trombone", 2, {{25, 20}, {39, 20}}, {26, 35}, true, {{21, 31}, {43, 31}}, {32, 13}, {16, 16}},
    {"orco_b", "orco", 2, {{25, 24}, {39, 24}}, {32, 42}, false, {{20, 37}, {44, 37}}, {32, 19}, {42, 18}},
};

/* ...and the friends of the fourth (0.11.0; anchors measured on tools/art/tale4.py) */
static const client_t FRIENDS4[CH_COUNT] = {
    {"mo_robottone_b", "robottone", 2, {{25, 13}, {39, 13}}, {32, 25}, false, {{20, 24}, {44, 24}}, {32, 8}, {44, 10}},
    {"mo_saltamolla_b", "saltamolla", 2, {{26, 16}, {38, 16}}, {32, 26}, false, {{21, 24}, {43, 24}}, {32, 10}, {44, 12}},
    {"mo_dinozzo_b", "dinozzo", 2, {{25, 14}, {39, 14}}, {32, 31}, false, {{19, 26}, {45, 26}}, {32, 9}, {46, 12}},
    {"mo_trottolina_b", "trottolina", 2, {{27, 17}, {37, 17}}, {32, 27}, false, {{23, 25}, {41, 25}}, {32, 12}, {42, 13}},
    {"re_b", "re", 2, {{19, 23}, {29, 23}}, {24, 32}, true, {{16, 29}, {32, 29}}, {24, 15}, {35, 17}},
};
#define MAX_CLIENTS (NCLIENTS + 3 * CH_COUNT)

/* how many friends of an adventure are free: all once it is behind, the places done in the current one */
static int friends_free(int arc)
{
    if (G.prog.arc > arc)
        return CH_COUNT;
    return G.prog.arc == arc ? clampi(G.prog.chapter, 0, CH_COUNT) : 0;
}

/* who comes to the salon: Deva's friends only. With the tale on, a monster
   comes once she has freed it, the witch (the wizard) once they are good, so
   the salon never tells the tale before it happens; the Mago Pistacchio always.
   Without the tale: all the friends of the first adventure. */
static int salon_clients(const client_t *out[MAX_CLIENTS])
{
    int n = 0;
    for (int i = 0; i < NCLIENTS; i++) {
        const char *id = CLIENTS[i].id;
        bool ok;
        if (!G.cfg.story || !strcmp(id, "mago"))
            ok = true;
        else if (!strcmp(id, "strega"))
            ok = story_arc_done(ARC_GRIGIO);
        else /* the four monsters, in the order of the places of the first adventure */
            ok = story_arc() > ARC_GRIGIO || G.prog.chapter > i;
        if (ok)
            out[n++] = &CLIENTS[i];
    }
    for (int i = 0; i < friends_free(ARC_NOTTE); i++)
        if (i != CH_VILLAIN || story_arc_done(ARC_NOTTE))
            out[n++] = &FRIENDS2[i];
    for (int i = 0; i < friends_free(ARC_MUSICA); i++)
        if (i != CH_VILLAIN || story_arc_done(ARC_MUSICA))
            out[n++] = &FRIENDS3[i];
    for (int i = 0; i < friends_free(ARC_GIOCHI); i++)
        if (i != CH_VILLAIN || story_arc_done(ARC_GIOCHI))
            out[n++] = &FRIENDS4[i];
    return n;
}

static const char *intro_voice(void)
{
    const client_t *c[MAX_CLIENTS];
    return salon_clients(c) > 1 ? "tm_intro" : "tm_intro_mago"; /* nobody freed yet: only the wizard */
}

/* the monsters of the duels, under the spell, in each adventure */
static const client_t FOES[ARC_COUNT][CH_COUNT] = {
    {{"mo_ciuffone_c", "ciuffone", 2, {{24, 27}, {40, 27}}, {32, 44}, false, {{16, 38}, {48, 38}}, {32, 16}, {24, 14}},
     {"mo_melmoso_c", "melmoso", 1, {{32, 21}, {32, 21}}, {32, 46}, false, {{17, 40}, {47, 40}}, {32, 14}, {22, 16}},
     {"mo_rocciolo_c", "rocciolo", 2, {{24, 29}, {40, 29}}, {32, 44}, false, {{15, 38}, {49, 38}}, {32, 17}, {46, 22}},
     {"mo_tuonello_c", "tuonello", 2, {{24, 26}, {40, 26}}, {32, 39}, false, {{16, 35}, {48, 35}}, {32, 10}, {44, 14}},
     {"strega_c", "strega", 2, {{21, 29}, {31, 29}}, {26, 39}, true, {{18, 35}, {34, 35}}, {26, 18}, {33, 19}}},
    {{"mo_polpone_c", "polpone", 2, {{24, 22}, {40, 22}}, {32, 37}, false, {{18, 31}, {46, 31}}, {32, 8}, {22, 10}},
     {"mo_lumacone_c", "lumacone", 2, {{7, 9}, {22, 9}}, {15, 44}, true, {{9, 45}, {22, 45}}, {15, 30}, {48, 21}},
     {"mo_nevone_c", "nevone", 2, {{26, 23}, {38, 23}}, {32, 35}, false, {{22, 31}, {42, 31}}, {32, 12}, {40, 12}},
     {"mo_fumino_c", "fumino", 2, {{25, 16}, {39, 16}}, {32, 33}, false, {{21, 26}, {43, 26}}, {32, 11}, {42, 10}},
     {"stregone_c", "stregone", 2, {{22, 26}, {30, 26}}, {26, 36}, true, {{19, 33}, {33, 33}}, {26, 17}, {33, 18}}},
    {{"mo_caramellone_c", "caramellone", 2, {{25, 18}, {39, 18}}, {32, 31}, false, {{19, 28}, {45, 28}}, {32, 8}, {45, 10}},
     {"mo_fungone_c", "fungone", 2, {{26, 33}, {38, 33}}, {32, 47}, false, {{22, 42}, {42, 42}}, {32, 6}, {46, 12}},
     {"mo_ranocchione_c", "ranocchione", 2, {{20, 17}, {44, 17}}, {32, 40}, false, {{17, 35}, {47, 35}}, {32, 21}, {50, 16}},
     {"mo_trombone_c", "trombone", 2, {{25, 21}, {39, 21}}, {26, 35}, true, {{21, 31}, {43, 31}}, {32, 13}, {16, 16}},
     {"orco_c", "orco", 2, {{25, 24}, {39, 24}}, {32, 41}, false, {{20, 37}, {44, 37}}, {32, 19}, {42, 18}}},
    {{"mo_robottone_c", "robottone", 2, {{25, 14}, {39, 14}}, {32, 24}, false, {{20, 24}, {44, 24}}, {32, 8}, {44, 10}},
     {"mo_saltamolla_c", "saltamolla", 2, {{26, 17}, {38, 17}}, {32, 26}, false, {{21, 24}, {43, 24}}, {32, 10}, {44, 12}},
     {"mo_dinozzo_c", "dinozzo", 2, {{24, 16}, {40, 16}}, {32, 33}, false, {{19, 26}, {45, 26}}, {32, 9}, {46, 12}},
     {"mo_trottolina_c", "trottolina", 2, {{27, 18}, {37, 18}}, {32, 27}, false, {{23, 25}, {41, 25}}, {32, 12}, {42, 13}},
     {"re_c", "re", 2, {{19, 23}, {29, 23}}, {24, 32}, true, {{16, 29}, {32, 29}}, {24, 15}, {35, 17}}},
};

#define MAX_WISH 3

static struct {
    const client_t *c;
    const client_t *last;      /* the client of the question before */
    bool greet;                /* a new client sits down: it says hello */
    bool heard;                /* every wish has been said at least once (level 4-5: "e adesso?") */
    int nclients;              /* how many can come today */
    int wish[MAX_WISH], nwish; /* items it wants, in the order they were said */
    bool applied[MAX_WISH];
    int opt[3];
    bool stepped;              /* ask() right after a step: remind briefly */
    int face_t;                /* sparkles on the face, frames */
} T;

static int variants(int slot) /* how many items share this slot */
{
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        n += (int)ITEMS[i].slot == slot;
    return n;
}

static void shuffle(int *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(0, i), t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

static bool wished(int item)
{
    for (int k = 0; k < T.nwish; k++)
        if (T.wish[k] == item)
            return true;
    return false;
}

/* three cards: the wish of this step and two items it did not ask for */
static void deal(quiz_t *q)
{
    int target = T.wish[q->step], slot = ITEMS[target].slot;
    int same[16], ns = 0, other[16], no = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (i == target || wished(i))
            continue;
        if ((int)ITEMS[i].slot == slot)
            same[ns++] = i;
        else
            other[no++] = i;
    }
    shuffle(same, ns);
    shuffle(other, no);
    int want_same = q->level == 1 ? 0 : (q->level == 2 || q->level == 4 ? 2 : 1);
    int d[2], nd = 0;
    for (int i = 0; i < ns && nd < want_same; i++)
        d[nd++] = same[i];
    for (int i = 0; i < no && nd < 2; i++)
        d[nd++] = other[i];
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        T.opt[i] = i == q->correct ? target : d[k++];
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        T.last = NULL;
    if (q->duel) {
        int ch = story_chapter() < CH_COUNT ? story_chapter() : CH_COUNT - 1;
        T.c = &FOES[story_arc()][ch];
    } else {
        const client_t *avail[MAX_CLIENTS];
        int n = salon_clients(avail);
        do
            T.c = avail[rng_range(0, n - 1)];
        while (n > 1 && T.c == T.last);
        T.greet = T.c != T.last; /* the same one again (only the wizard today): no second "ciao" */
        T.nclients = n;
        T.last = T.c;
    }
    /* the wishes: different slots; level 2 and 4 pick slots that have lookalikes */
    T.nwish = level <= 2 ? 1 : (level <= 4 ? 2 : 3);
    int slots[6] = {SLOT_EYES, SLOT_LIPS, SLOT_BLUSH, SLOT_STICKER, SLOT_GLITTER, SLOT_HEAD}, ns = 6;
    if (level == 2 || level == 4) { /* eyes, lips, stickers, head: more than one colour or kind */
        ns = 0;
        for (int s = 0; s < 6; s++)
            if (variants(s) > 1)
                slots[ns++] = s;
    }
    shuffle(slots, ns);
    for (int k = 0; k < T.nwish; k++) {
        int cand[16], nc = 0;
        for (int i = 0; i < ITEM_COUNT; i++)
            if ((int)ITEMS[i].slot == slots[k])
                cand[nc++] = i;
        T.wish[k] = cand[rng_range(0, nc - 1)];
        T.applied[k] = false;
    }
    q->steps = T.nwish;
    q->card = QUIZ_CARD_PICTURE;
    T.heard = T.nwish == 1;
    T.stepped = false;
    T.face_t = 0;
    deal(q);
}

static void next_step(quiz_t *q)
{
    T.stepped = true;
    deal(q);
}

static void say_wish(int from, bool now)
{
    char id[40];
    for (int k = from; k < T.nwish; k++) {
        snprintf(id, sizeof(id), "%s%s", k == from ? "tm_" : "tm_e_", ITEMS[T.wish[k]].id);
        if (k == from && now)
            say(id);
        else
            say_then(id);
    }
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1) {
        sfx("poof");
        fx_sparkles(q->panel_cx, q->panel_cy, 18, 12);
        if (!q->duel && T.greet) {
            char id[40];
            snprintf(id, sizeof(id), "tm_ciao_%s", T.c->id);
            say(id);
        }
    }
    return q->t > 30 && !voice_busy();
}

static void ask(quiz_t *q)
{
    /* from memory: only "e adesso?" - if she has heard them all; an answer or an
       arrow can cut the list short, then the rest is said again */
    if (T.stepped && q->level >= 4 && T.heard) {
        T.stepped = false;
        say("tm_ancora");
        return;
    }
    T.stepped = false;
    say_wish(q->step, true);
}

static void hover(quiz_t *q, int i)
{
    char id[40];
    snprintf(id, sizeof(id), "t_%s", ITEMS[T.opt[i]].id);
    say(id);
}

static void right(quiz_t *q)
{
    /* thanks when the client leaves: every time, or at the end of the round if it stays */
    if (!q->duel && (T.nclients > 1 || q->question + 1 >= quiz_round_len())) {
        char id[40];
        snprintf(id, sizeof(id), "tm_grazie_%s", T.c->id);
        say_then(id);
    }
}

static void tick(quiz_t *q)
{
    if (!T.heard && T.nwish > 0) { /* the last wish of the list has started: she has heard them all
                                      (tick runs in the intro too, before the first question) */
        char id[40];
        snprintf(id, sizeof(id), "tm_e_%s", ITEMS[T.wish[T.nwish - 1]].id);
        if (voice_current() >= 0 && voice_current() == snd_find(SND_VOICE, id))
            T.heard = true;
    }
    /* the item just chosen goes on the face */
    if ((q->state == Q_STEP || q->state == Q_RIGHT) && !T.applied[q->step]) {
        T.applied[q->step] = true;
        T.face_t = 1;
        sfx("pennello");
    }
    if (T.face_t > 0 && ++T.face_t > 40)
        T.face_t = 0;
}

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("tm_aiuto"); /* "Ascolta bene..." */
        char id[40];
        snprintf(id, sizeof(id), "tm_%s", ITEMS[T.wish[q->step]].id);
        say_then(id);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ drawing */

static int K2 = 2;

static void at(int ox, int oy, const int p[2], const char *sprite, int dx, int dy)
{
    const sprite_t *s = gfx_sprite(sprite);
    gfx_blit_scaled(s, ox + K2 * p[0] - K2 * s->w / 2 + dx, oy + K2 * p[1] + dy, K2, 0);
}

static void draw_makeup(int ox, int oy)
{
    const client_t *c = T.c;
    char name[40];
    for (int k = 0; k < T.nwish; k++) {
        if (!T.applied[k])
            continue;
        const item_t *it = &ITEMS[T.wish[k]];
        switch (it->slot) {
        case SLOT_EYES:
            snprintf(name, sizeof(name), "%s_%s", c->eyes == 1 ? "mk_ombrettog" : "mk_ombretto", it->variant);
            for (int e = 0; e < c->eyes; e++) {
                const sprite_t *s = gfx_sprite(name);
                at(ox, oy, c->eye[e], name, 0, -K2 * (s->h - 1));
            }
            break;
        case SLOT_LIPS: {
            snprintf(name, sizeof(name), "%s_%s", c->small ? "mk_rossettos" : "mk_rossetto", it->variant);
            const sprite_t *s = gfx_sprite(name);
            at(ox, oy, c->mouth, name, 0, -K2 * s->h / 2);
            break;
        }
        case SLOT_BLUSH:
            for (int e = 0; e < 2; e++)
                at(ox, oy, c->cheek[e], "mk_guance", 0, -K2 * 3);
            break;
        case SLOT_STICKER:
            snprintf(name, sizeof(name), "mk_adesivo_%s", it->variant);
            at(ox, oy, c->cheek[1], name, K2 * 2, -K2 * 11);
            break;
        case SLOT_GLITTER: {
            static const int spots[3][2] = {{-18, -10}, {17, -4}, {-14, 8}};
            const sprite_t *sp = gfx_sprite("sparkle");
            int mx = (c->eye[0][0] + c->eye[c->eyes - 1][0]) / 2, my = c->eye[0][1];
            for (int i = 0; i < 3; i++)
                if (((G.frame / 10) + i) % 3 != 0)
                    gfx_blit_scaled(sp, ox + K2 * (mx + spots[i][0]), oy + K2 * (my + spots[i][1]), K2, 0);
            break;
        }
        case SLOT_HEAD:
            if (!strcmp(it->variant, "tiara"))
                at(ox, oy, c->top, "hero_tiara", 0, -K2 * 6);
            else
                at(ox, oy, c->flower, "hero_flower", 0, -K2 * 5);
            break;
        }
    }
}

static void draw(quiz_t *q)
{
    const sprite_t *s = gfx_sprite(T.c->sprite);
    int ox = q->panel_cx - s->w * K2 / 2 + 24, oy = 34; /* room for the bubble on the left */
    if (q->state == Q_PREPARE && q->t < 8) /* appears with a puff */
        return;
    int bob = swing((int)G.frame, 90, 1);
    if (q->state == Q_RIGHT && q->t < 80) /* happy with its new look: a little bounce */
        bob = ((q->t / 6) & 1) * 4;
    gfx_set_clip(84, 32, 298, 162);
    gfx_blit_scaled(s, ox, oy - bob, K2, 0);
    draw_makeup(ox, oy - bob);
    gfx_reset_state();
    if (T.face_t > 0 && T.face_t % 8 == 1)
        fx_sparkles(q->panel_cx + 24, oy + 60, 8, 6);
    /* the wish of this step in a speech bubble: a picture (level 1), its shape only
       (levels 2-4: the colour must be heard); in the duels too */
    if (q->level <= 4 && q->state != Q_PREPARE) {
        const sprite_t *b = gfx_sprite("fumetto");
        int bx = 88, by = 36;
        gfx_blit(b, bx, by, 0);
        char name[40];
        snprintf(name, sizeof(name), "rw_%s", ITEMS[T.wish[q->step]].id);
        const sprite_t *ic = gfx_sprite(name);
        int ix = bx + 26 - ic->w / 2, iy = by + 19 - ic->h / 2;
        if (q->level == 1 || q->state == Q_RIGHT || q->assisted)
            gfx_blit(ic, ix, iy, 0);
        else
            ui_silhouette(ic, ix, iy);
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    char name[40];
    snprintf(name, sizeof(name), "rw_%s", ITEMS[T.opt[i]].id);
    const sprite_t *s = gfx_sprite(name);
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", ITEMS[T.opt[i]].id); }

static const char *subject(quiz_t *q)
{
    static char s[96];
    size_t pos = (size_t)snprintf(s, sizeof(s), "trucco:%s:", T.c->id);
    for (int k = 0; k < T.nwish && pos < sizeof(s); k++)
        pos += (size_t)snprintf(s + pos, sizeof(s) - pos, "%s%s", k ? "+" : "", ITEMS[T.wish[k]].id);
    return s;
}

const quiz_def_t QUIZ_TRUCCO = {
    .name = "trucco",
    .game = GAME_TRUCCO,
    .bg = "bg_trucco",
    .intro = "tm_intro",
    .intro_voice = intro_voice,
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

static void enter(void) { quiz_enter(&QUIZ_TRUCCO); }

const scene_t SCENE_TRUCCO = {enter, quiz_update, quiz_draw};
