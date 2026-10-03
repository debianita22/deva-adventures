/* Deva's Awesome Adventures - the duel of the tale: questions borrowed from
 * the card games, in the monster's place.
 *
 * The monster floats at the top left inside a ring of spell orbs; Deva stands
 * below with her wand. A right answer is a spell: she raises the wand, a
 * trail of sparkles flies up and an orb breaks, the monster squirms ("mi fai
 * il solletico!"). A wrong answer: the monster dodges with a boing, and the
 * usual gentle help of the games goes on. When every orb is broken the spell
 * is over. The villain has one orb more, and a last question that is not a
 * spell: the witch cries, the wizard trembles - how do they feel?
 * SPDX-License-Identifier: MIT
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"
#include "story.h"

#define MAX_ORBS 6
#define RING_X 40  /* centre of the ring of magic bubbles, where the monster floats */
#define RING_Y 64
#define RING_R 37
#define BEAM 14    /* frames of the sparkle trail */

static struct {
    int ch, orbs, broken;
    const quiz_def_t *pool[6];
    int npool, last;
    int bag[6], nbag, bag_pos; /* the pool shuffled: every game of the place once, then again */
    bool bridge;               /* the villain: the last orb is "how does Deva feel?" (pool[0]) */
    int cast_t;    /* frames since the spell started, -1 = none */
    int hit_t;     /* the monster squirms */
    int dodge_t;   /* the monster dodges */
    int flash_t;   /* lightning in the castle */
    int target;    /* orb being broken */
    int spin;      /* ring rotation, degrees */
    bool crying;   /* the witch's last question */
} S;

static bool witch(void) { return story_villain(S.ch); } /* the witch, or the wizard */

/* ---- the villain's last question: how does she (he) feel? */

typedef struct {
    const char *sprite, *intro, *question, *hint, *subject;
    const char *feel[3];  /* the right one first; card sprites emo_<feel> */
    const char *voice[3]; /* what each card says */
} last_t;

static const last_t LAST[ARC_COUNT] = {
    {"strega_pianto", "fn_pianto", "fn_domanda", "emo_d_triste", "strega:triste",
     {"triste", "felice", "sorpresa"}, {"emo_triste", "emo_felice", "emo_sorpresa"}},
    {"stregone_paura", "st2_paura", "st2_domanda", "emo_d_spaventata", "stregone:spaventato",
     {"spaventata", "felice", "arrabbiata"}, {"st2_emo_spaventato", "emo_felice", "st2_emo_arrabbiato"}},
    {"orco_brontola", "st3_sbuffa", "st3_domanda", "emo_d_arrabbiata", "orco:arrabbiato",
     {"arrabbiata", "felice", "sorpresa"}, {"st2_emo_arrabbiato", "emo_felice", "st3_emo_sorpreso"}},
    {"re_piange", "st4_piange", "st4_domanda", "emo_d_triste", "re:triste",
     {"triste", "felice", "sorpresa"}, {"emo_triste", "emo_felice", "st3_emo_sorpreso"}},
};
static const last_t *last(void) { return &LAST[story_arc()]; }
static int s_opt[3];

static void pianto_setup(quiz_t *q, int level)
{
    (void)level;
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    int others[2] = {1, 2};
    if (rng_range(0, 1)) {
        others[0] = 2;
        others[1] = 1;
    }
    for (int i = 0, k = 0; i < 3; i++)
        s_opt[i] = i == q->correct ? 0 : others[k++];
}

/* "the witch starts to cry" is told before the cards can move, so that the
   question after it is not cut short by an arrow pressed during the tale */
static bool pianto_prepare(quiz_t *q)
{
    if (q->t == 1) { /* the spell is broken: no more duel music, the quiet one of the ending */
        music_play(snd_find(SND_MUSIC, "nanna"));
        say(last()->intro);
    }
    return q->t > 30 && !voice_busy();
}

static void pianto_ask(quiz_t *q) { say(last()->question); }

static void pianto_hover(quiz_t *q, int i) { say(last()->voice[s_opt[i]]); }

static bool pianto_hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say(last()->hint);
        return false;
    }
    say("emo_scegli");
    return true;
}

static void pianto_draw(quiz_t *q)
{
    /* the villain, big, crying (trembling, stamping) in the panel */
    const sprite_t *s = gfx_sprite(last()->sprite);
    int dx = story_arc() == ARC_NOTTE ? (((G.frame / 2) & 1) ? 1 : -1) : 0;
    int dy = story_arc() == ARC_MUSICA ? ((G.frame % 24) < 5 ? -3 : 0) : -swing((int)G.frame, 90, 1);
    gfx_set_clip(84, 32, 298, 162);
    gfx_blit_scaled(s, q->panel_cx - s->w + dx, 36 + dy, 2, 0);
    gfx_reset_state();
}

static void pianto_card(quiz_t *q, int i, int cx, int cy, int style)
{
    char name[24];
    snprintf(name, sizeof(name), "emo_%s", last()->feel[s_opt[i]]);
    const sprite_t *s = gfx_sprite(name);
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void pianto_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", last()->feel[s_opt[i]]); }
static const char *pianto_subject(quiz_t *q) { return last()->subject; }

static const quiz_def_t PIANTO = {
    .name = "strega",
    .game = GAME_EMOZIONI,
    .gentle = true,
    .bg = "bg_castello",
    .card = QUIZ_CARD_PICTURE,
    .setup = pianto_setup,
    .prepare = pianto_prepare,
    .ask = pianto_ask,
    .hover = pianto_hover,
    .hint = pianto_hint,
    .draw = pianto_draw,
    .card_draw = pianto_card,
    .card_value = pianto_value,
    .subject = pianto_subject,
};

static const quiz_def_t BRONTOLA = { /* the same, for the ogre of the third adventure */
    .name = "orco",
    .game = GAME_EMOZIONI,
    .gentle = true,
    .bg = "bg_casa_orco",
    .card = QUIZ_CARD_PICTURE,
    .setup = pianto_setup,
    .prepare = pianto_prepare,
    .ask = pianto_ask,
    .hover = pianto_hover,
    .hint = pianto_hint,
    .draw = pianto_draw,
    .card_draw = pianto_card,
    .card_value = pianto_value,
    .subject = pianto_subject,
};

static const quiz_def_t CAPRICCIO = { /* the same, for the little king of the fourth adventure */
    .name = "re",
    .game = GAME_EMOZIONI,
    .gentle = true,
    .bg = "bg_castello_giochi",
    .card = QUIZ_CARD_PICTURE,
    .setup = pianto_setup,
    .prepare = pianto_prepare,
    .ask = pianto_ask,
    .hover = pianto_hover,
    .hint = pianto_hint,
    .draw = pianto_draw,
    .card_draw = pianto_card,
    .card_value = pianto_value,
    .subject = pianto_subject,
};

static const quiz_def_t PAURA = { /* the same, for the wizard of the second adventure */
    .name = "stregone",
    .game = GAME_EMOZIONI,
    .gentle = true,
    .bg = "bg_torre",
    .card = QUIZ_CARD_PICTURE,
    .setup = pianto_setup,
    .prepare = pianto_prepare,
    .ask = pianto_ask,
    .hover = pianto_hover,
    .hint = pianto_hint,
    .draw = pianto_draw,
    .card_draw = pianto_card,
    .card_value = pianto_value,
    .subject = pianto_subject,
};

/* ---- the duel */

static const quiz_def_t *def_of(int game)
{
    switch (game) {
    case GAME_CONTA: return &QUIZ_CONTA;
    case GAME_PAROLE: return &QUIZ_PAROLE;
    case GAME_SEQUENZE: return &QUIZ_SEQUENZE;
    case GAME_EMOZIONI: return &QUIZ_EMOZIONI;
    case GAME_STORIE: return &QUIZ_STORIE;
    case GAME_GINNASTICA: return &QUIZ_SALTI;
    case GAME_TRUCCO: return &QUIZ_TRUCCO;
    case GAME_FORME: return &QUIZ_FORME;
    case GAME_NOME: return &QUIZ_NOME;
    case GAME_BALLA: return &QUIZ_PASSI;
    case GAME_RITMO: return &QUIZ_TAMBURO;
    case GAME_DOVE: return &QUIZ_DOVE;
    case GAME_MEMORY: return &QUIZ_SPARITO;
    case GAME_LETTERE: return &QUIZ_LETTERE;
    case GAME_OMBRE: return &QUIZ_OMBRE;
    case GAME_SENTIERO: return &QUIZ_STRADA;
    case GAME_NEGOZIO: return &QUIZ_NEGOZIO;
    case GAME_MISURE: return &QUIZ_MISURE;
    default: return NULL;
    }
}

static void refill_bag(void)
{
    S.nbag = 0;
    for (int i = S.bridge ? 1 : 0; i < S.npool; i++) /* the bridge question comes last, by itself */
        S.bag[S.nbag++] = i;
    for (int i = S.nbag - 1; i > 0; i--) {
        int j = rng_range(0, i), t = S.bag[i];
        S.bag[i] = S.bag[j];
        S.bag[j] = t;
    }
    if (S.nbag > 1 && S.bag[0] == S.last) { /* never the same game twice in a row */
        int t = S.bag[0];
        S.bag[0] = S.bag[1];
        S.bag[1] = t;
    }
    S.bag_pos = 0;
}

static const quiz_def_t *next(int question)
{
    if (question >= S.orbs) {
        static const quiz_def_t *const LAST_Q[ARC_COUNT] = {&PIANTO, &PAURA, &BRONTOLA, &CAPRICCIO};
        S.crying = true;
        return LAST_Q[story_arc()];
    }
    /* just before "how does the witch (the wizard, the ogre, the king) feel?", the same feeling on Deva */
    if (S.bridge && question == S.orbs - 1) {
        static const char *const BRIDGE[ARC_COUNT] = {"faccia:triste", "storia:luna", "storia:letto",
                                                      "storia:palloncino"};
        emozioni_force(BRIDGE[story_arc()]);
        S.last = 0;
        return S.pool[0];
    }
    /* each game of the place once before any comes back: a duel shows them all */
    if (S.bag_pos >= S.nbag)
        refill_bag();
    S.last = S.bag[S.bag_pos++];
    return S.pool[S.last];
}

static void orb_pos(int i, int *x, int *y)
{
    /* evenly around the ring, turning slowly */
    int a = S.spin + i * 360 / S.orbs;
    double r = a * 3.14159265 / 180.0;
    *x = RING_X + (int)(RING_R * cos(r));
    *y = RING_Y + (int)(RING_R * 0.9 * sin(r));
}

/* the monster is saying one of its lines (it bounces with its voice) */
static bool foe_speaks(void)
{
    static const char *LINES[3] = {"sf_colpo_1", "sf_colpo_2", "sf_colpo_3"};
    sound_t v = voice_current();
    if (v < 0)
        return false;
    for (int i = 0; i < 3; i++)
        if (v == snd_find(SND_VOICE, LINES[i]))
            return true;
    return witch() && v == snd_find(SND_VOICE, story_arc_def()->v_hit);
}

static void foe_draw(void)
{
    const char *name = story_foe_sprite(S.ch, S.hit_t > 0 ? 'r' : 'c');
    const sprite_t *s = gfx_sprite(name);
    /* 0.13: it floats softly, breathes and blinks; tickled it squirms (squashed and stretched in
       turn); dodging it leans away, long and thin */
    int dx = 0, dy = swing((int)G.frame, 90, 3), sdy = 0, sdx = 0;
    if (S.hit_t > 0) {
        dx = ((S.hit_t / 2) & 1) ? 2 : -2;
        sdy = wave(S.hit_t, 8, 4);
        sdx = -sdy;
    }
    if (S.dodge_t > 0) {
        int a = 10 - absi(S.dodge_t - 10);
        dx = (S.dodge_t > 10 ? 1 : -1) * a * 2;
        sdy = a / 2;
        sdx = -a / 2;
    }
    if (S.hit_t > 0 && ((S.hit_t / 3) & 1))
        gfx_set_tint(rgb565(0xff, 0xff, 0xff), 150);
    actor_draw(name, RING_X + dx, RING_Y + s->h - s->h / 2 + 4 - dy, 1, 0, foe_speaks() ? voice_level() : 0, sdy,
               sdx);
    gfx_set_tint(0, 0);
}

/* the panel of the questions (0.12): the place shows faintly through it, the frame has the colours
   of the adventure and its treasures in the corners, and a tail points at the monster who asks */
static void duel_panel(void)
{
    static const uint8_t COL[ARC_COUNT][2][3] = {{{0xa8, 0x6a, 0xe8}, {0xdc, 0xbc, 0xff}},  /* the gems */
                                                 {{0x3a, 0x5a, 0xd8}, {0x9c, 0xbc, 0xff}},  /* the stars */
                                                 {{0x22, 0xa8, 0x9c}, {0x96, 0xf0, 0xdc}},  /* the notes */
                                                 {{0xf0, 0x5a, 0x5a}, {0xff, 0xc0, 0x80}}}; /* the keys */
    const uint8_t(*c)[3] = COL[story_arc()];
    uint16_t main = rgb565(c[0][0], c[0][1], c[0][2]), light = rgb565(c[1][0], c[1][1], c[1][2]);
    uint16_t ink = rgb565(0x3b, 0x1f, 0x4a), paper = rgb565(0xff, 0xfb, 0xff);
    const int x = 80, y = 28, w = 223, h = 139;
    gfx_fade_round_rect(x, y, w, h, 9, paper, 228);
    gfx_frame_round(x, y, w, h, 9, 3, main, light, ink);
    if (!S.crying) { /* the tail of a speech bubble, from the panel to the monster */
        const int tip_x = 66, tip_y = RING_Y - 2, base_x = x + 3, half = 9;
        for (int xx = tip_x; xx <= base_x; xx++) {
            int k = (xx - tip_x) * half / (base_x - tip_x), y0 = tip_y - k / 2 - 1, y1 = tip_y + k;
            gfx_fill_rect(xx, y0, 1, y1 - y0 + 1, xx >= x ? paper : rgb565(0xf6, 0xf0, 0xfa));
            gfx_fill_rect(xx, y0 - 1, 1, 1, ink);
            gfx_fill_rect(xx, y1 + 1, 1, 1, ink);
        }
        gfx_fill_rect(tip_x - 1, tip_y - 1, 1, 2, ink);
    }
    for (int i = 0; i < 4; i++) { /* the treasures of the adventure, in the corners */
        const sprite_t *g = gfx_sprite(story_ch(i)->gem);
        int gx = i & 1 ? x + w - 6 - g->w / 2 : x + 6 - g->w / 2, gy = i & 2 ? y + h - 6 - g->h / 2 : y + 6 - g->h / 2;
        if (i == 0 && !S.crying)
            gx += 4; /* clear of the tail */
        gfx_blit(g, gx, gy, 0);
    }
}

static void draw_back(quiz_t *q)
{
    char bg[32];
    snprintf(bg, sizeof(bg), "bg_%s", story_ch(S.ch)->place);
    gfx_draw_bg(gfx_image_spell(bg, story_arc())); /* the place is under the spell until the duel is won (0.12) */
    duel_panel();
    if (!S.crying) {
        /* orbs behind the monster (the far half of the ring), then the monster, then the near half */
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1)
                foe_draw();
            for (int i = S.broken; i < S.orbs; i++) {
                int x, y;
                orb_pos(i, &x, &y);
                bool far = y < RING_Y;
                if (far != (pass == 0))
                    continue;
                const sprite_t *o = gfx_sprite("bolla_magica");
                gfx_blit(o, x - o->w / 2, y - o->h / 2, 0);
            }
        }
    }
    /* Deva with the wand */
    hero_draw(&G.hero, &G.prog);
    int hx, hy;
    hero_hand(&G.hero, 1, &hx, &hy);
    gfx_blit(gfx_sprite("bacchetta"), hx - 4, hy - 18, 0);
}

static void draw_front(quiz_t *q)
{
    if (S.cast_t >= 0 && S.cast_t <= BEAM + 2) {
        /* the sparkle trail from the wand's star to the orb */
        int hx, hy, tx, ty;
        hero_hand(&G.hero, 1, &hx, &hy);
        hy -= 16;
        orb_pos(S.target, &tx, &ty);
        static const uint8_t RGB[5][3] = {
            {0xff, 0x6d, 0xb0}, {0xff, 0xc8, 0x2e}, {0x4f, 0xd6, 0xc0}, {0x74, 0xb8, 0xff}, {0xb3, 0x76, 0xec}};
        const sprite_t *tw[2] = {gfx_sprite("tw_2"), gfx_sprite("tw_3")};
        int n = clampi(S.cast_t, 0, BEAM);
        for (int i = 0; i <= n; i++) { /* a rainbow of twinkles */
            int x = hx + (tx - hx) * i / BEAM, y = hy + (ty - hy) * i / BEAM;
            if (((i + S.cast_t) & 1) == 0) {
                const sprite_t *sp = tw[(i / 2) & 1];
                const uint8_t *c = RGB[(i + S.cast_t / 2) % 5];
                gfx_set_tint(rgb565(c[0], c[1], c[2]), 110);
                gfx_blit(sp, x - sp->w / 2, y - sp->h / 2, 0);
            }
        }
        gfx_set_tint(0, 0);
    }
    if (witch() && S.flash_t > 0 && S.flash_t < 4)
        gfx_fade(rgb565(0xff, 0xff, 0xff), 120);
}

static void right(quiz_t *q)
{
    if (S.crying || S.broken >= S.orbs)
        return;
    S.target = S.broken;
    S.cast_t = 0;
    hero_move(&G.hero, POSE_UP, false, 40);
    sfx("incantesimo");
}

static void wrong(quiz_t *q)
{
    if (S.crying)
        return;
    S.dodge_t = 20;
    sfx("schivata");
}

static void tick(quiz_t *q)
{
    S.spin = (S.spin + 1) % 360;
    if (G.frame % 50 == 25) { /* the star of the wand twinkles */
        int hx, hy;
        hero_hand(&G.hero, 1, &hx, &hy);
        fx_twinkle(hx + rng_range(-1, 1), hy - 17, 0);
    }
    if (!S.crying && S.broken < S.orbs && G.frame % 40 == 5) { /* a glint on one of the orbs */
        int x, y;
        orb_pos(rng_range(S.broken, S.orbs - 1), &x, &y);
        fx_twinkle(x + 2, y - 3, 0);
    }
    if (S.hit_t > 0)
        S.hit_t--;
    if (S.dodge_t > 0)
        S.dodge_t--;
    if (S.cast_t >= 0) {
        S.cast_t++;
        if (S.cast_t == BEAM) { /* the orb breaks */
            int x, y;
            orb_pos(S.target, &x, &y);
            S.broken++;
            sfx("rottura");
            fx_sparkles(x, y, 12, 14);
            fx_burst(x, y, 12);
            S.hit_t = 24;
            BOT("duel orb=%d/%d\n", S.broken, S.orbs);
            /* now and then the monster says something about it */
            if (S.broken % 2 == 1 || S.broken == S.orbs) {
                static const char *LINES[3] = {"sf_colpo_1", "sf_colpo_2", "sf_colpo_3"};
                say_then(witch() ? story_arc_def()->v_hit : LINES[(S.broken / 2) % 3]);
            }
        }
        if (S.cast_t > 60)
            S.cast_t = -1;
    }
    if (S.flash_t > 0)
        S.flash_t--;
    else if (witch() && !S.crying && story_arc() <= ARC_NOTTE && rng_range(0, 600) == 0) {
        S.flash_t = 12; /* lightning outside, now and then; not when she cries (he trembles); */
        sfx("tuono");   /* the ogre's house and the toy castle are quiet */
    }
}

static void done(void)
{
    if (witch()) {
        G.prog.chapter = CH_COUNT;
        G.prog.charge = 0;
        game_save();
        BOT("duel won=%s\n", story_ch(S.ch)->foe);
        racconto_play(R_FINALE, SC_MENU);
        return;
    }
    BOT("duel won=%s\n", story_ch(S.ch)->foe);
    G.prog.chapter++;
    G.prog.charge = 0;
    game_save();
    racconto_play(R_VITTORIA, SC_MAPPA);
}

static quiz_host_t HOST = {
    .next = next,
    .draw_back = draw_back,
    .draw_front = draw_front,
    .right = right,
    .wrong = wrong,
    .tick = tick,
    .done = done,
};

void sfida_start(void) { game_goto(SC_SFIDA); }

static void enter(void)
{
    memset(&S, 0, sizeof(S));
    S.ch = clampi(story_chapter(), 0, CH_COUNT - 1);
    S.cast_t = -1;
    S.last = -1;
    /* the chapter's games that she can already play (the first three always can);
       the villain's first one (Emozioni) always: its question on Deva's feelings
       ("sad", "afraid of the dark") comes last, just before "how does the witch feel?" */
    for (int i = 0; i < 6 && story_ch(S.ch)->games[i] >= 0; i++) {
        int g = story_ch(S.ch)->games[i];
        if ((game_is_unlocked(&G.prog, &G.cfg, g) || (witch() && i == 0)) && def_of(g))
            S.pool[S.npool++] = def_of(g);
    }
    if (S.npool == 0)
        S.pool[S.npool++] = &QUIZ_CONTA;
    S.orbs = clampi(G.cfg.questions_per_round, 3, 5) + (witch() ? 1 : 0);
    S.bridge = witch() && S.npool > 1 && S.pool[0] == &QUIZ_EMOZIONI;
    refill_bag();
    HOST.questions = S.orbs + (witch() ? 1 : 0);
    hero_init(&G.hero, 30, 214);
    music_play(snd_find(SND_MUSIC, "sfida"));
    BOT("duel foe=%s orbs=%d games=%d\n", story_ch(S.ch)->foe, S.orbs, S.npool);
    quiz_enter_host(&HOST);
}

const scene_t SCENE_SFIDA = {enter, quiz_update, quiz_draw};
