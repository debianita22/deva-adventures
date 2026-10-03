/* Deva's Awesome Adventures - "Forme e colori", in the laboratory of Mago
 * Pistacchio: he needs shapes for his potion ("Trova il quadrato blu!"); she
 * picks it among three cards and it flies into the cauldron, which takes its
 * colour.
 *
 * It trains the names of shapes and colours, looking at two things at once,
 * and "not" / "the odd one out". Levels: 1 the shape (circle, square,
 * triangle; all cards one colour); 2 the colour (one shape, four colours);
 * 3 shape and colour, where each wrong card shares one of them; 4 the same
 * with six shapes and seven colours; 5 also "qual è diverso?", "quale non è
 * un cerchio?", "quale non è rosso?". Guided help: what the shape is like
 * ("il triangolo ha tre punte"), what the colour is like ("rosso come una
 * fragola").
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
#include "ui.h"

enum { F_CERCHIO, F_QUADRATO, F_TRIANGOLO, F_RETTANGOLO, F_STELLA, F_CUORE, F_COUNT };
static const char *SHAPES[F_COUNT] = {"cerchio", "quadrato", "triangolo", "rettangolo", "stella", "cuore"};

enum { C_ROSSO, C_BLU, C_GIALLO, C_VERDE, C_ARANCIONE, C_VIOLA, C_ROSA, C_COUNT };
static const char *COLORS[C_COUNT] = {"rosso", "blu", "giallo", "verde", "arancione", "viola", "rosa"};
static const uint8_t RGB[C_COUNT][3][3] = { /* base, light, dark: palette of tools/art/pixel.py */
    {{0xe5, 0x39, 0x35}, {0xff, 0x7a, 0x74}, {0xa8, 0x1c, 0x2c}},
    {{0x47, 0x77, 0xd1}, {0x74, 0xb8, 0xff}, {0x2b, 0x4a, 0x9a}},
    {{0xff, 0xd2, 0x3f}, {0xff, 0xf3, 0xa6}, {0xf0, 0xa0, 0x30}},
    {{0x4c, 0xc2, 0x5a}, {0xa8, 0xf0, 0x8a}, {0x2a, 0x8a, 0x45}},
    {{0xff, 0x7b, 0x2e}, {0xff, 0xb8, 0x7a}, {0xb8, 0x6b, 0x1c}},
    {{0x7d, 0x3f, 0xc4}, {0xb3, 0x76, 0xec}, {0x3a, 0x26, 0x66}},
    {{0xff, 0x93, 0xc6}, {0xff, 0xcb, 0xe3}, {0xf0, 0x55, 0x9e}},
};

enum { M_FORMA, M_COLORE, M_ENTRAMBI, M_DIVERSO, M_NON_FORMA, M_NON_COLORE };

typedef struct {
    int shape, color;
} card_t;

static struct {
    int mode;
    card_t opt[3];
    int shape, color;     /* what the question names (the odd one for M_DIVERSO) */
    int last_shape, last_color;
    int potion;           /* colour in the cauldron, -1 = clear */
    int fly_t;            /* the chosen shape flying into the cauldron, -1 = none */
    int bubble_t;
} F;

static uint16_t col(int c, int tone) { return rgb565(RGB[c][tone][0], RGB[c][tone][1], RGB[c][tone][2]); }

static int pick(int n, int avoid) /* 0..n-1, not avoid */
{
    int v;
    do
        v = rng_range(0, n - 1);
    while (v == avoid && n > 1);
    return v;
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0) {
        F.last_shape = F.last_color = -1;
        F.potion = -1;
    }
    F.fly_t = -1;
    int nshapes = level <= 2 ? 3 : (level == 3 ? 4 : F_COUNT);
    int ncolors = level == 1 ? C_COUNT : (level <= 4 ? 4 : C_COUNT); /* level 4: all the shapes, still 4 colours */
    int stella_ok = level >= 3; /* at level 3 the fourth shape is the star, not the rectangle */
    F.mode = level == 1 ? M_FORMA : (level == 2 ? M_COLORE : (level <= 4 ? M_ENTRAMBI : rng_range(M_ENTRAMBI, M_NON_COLORE)));
    int s = pick(nshapes, F.last_shape), c = pick(ncolors, F.last_color);
    if (stella_ok && level == 3 && s == F_RETTANGOLO)
        s = F_STELLA;
    F.shape = s;
    F.color = c;
    F.last_shape = s;
    F.last_color = c;
    card_t right = {s, c}, d1 = {s, c}, d2 = {s, c};
    switch (F.mode) {
    case M_FORMA: { /* three shapes, one colour */
        int others[F_COUNT], n = 0;
        for (int k = 0; k < nshapes; k++)
            if (k != s)
                others[n++] = k;
        int a = rng_range(0, n - 1), b;
        do
            b = rng_range(0, n - 1);
        while (b == a && n > 1);
        d1.shape = others[a];
        d2.shape = others[b];
        break;
    }
    case M_COLORE: { /* one shape, three colours */
        right.shape = d1.shape = d2.shape = F.shape = rng_range(0, F_COUNT - 1);
        d1.color = pick(ncolors, c);
        do
            d2.color = pick(ncolors, c);
        while (d2.color == d1.color);
        break;
    }
    case M_ENTRAMBI: /* one wrong card has the colour, the other the shape */
        d1.color = pick(ncolors, c);
        d2.shape = pick(nshapes, s);
        if (level == 3 && d2.shape == F_RETTANGOLO)
            d2.shape = s == F_STELLA ? F_CERCHIO : F_STELLA;
        break;
    case M_DIVERSO: /* two the same, one different in shape or in colour */
        if (rng_range(0, 1))
            right.shape = pick(nshapes, s);
        else
            right.color = pick(ncolors, c);
        F.shape = right.shape;
        F.color = right.color;
        break;
    case M_NON_FORMA: /* "quale non è un cerchio?": two circles, one not */
        d1.color = pick(ncolors, -1);
        d2.color = pick(ncolors, d1.color);
        right.shape = pick(nshapes, s);
        right.color = pick(ncolors, -1);
        break;
    default: /* "quale non è rosso?": two red, one not */
        d1.shape = pick(nshapes, -1);
        d2.shape = pick(nshapes, d1.shape);
        right.color = pick(ncolors, c);
        right.shape = pick(nshapes, -1);
        break;
    }
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    card_t d[2] = {d1, d2};
    for (int i = 0, k = 0; i < 3; i++)
        F.opt[i] = i == q->correct ? right : d[k++];
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("pop");
    return q->t > 20;
}

static void ask(quiz_t *q)
{
    char id[48];
    switch (F.mode) {
    case M_FORMA: snprintf(id, sizeof(id), "fc_s_%s", SHAPES[F.shape]); break;
    case M_COLORE: snprintf(id, sizeof(id), "fc_c_%s", COLORS[F.color]); break;
    case M_ENTRAMBI: snprintf(id, sizeof(id), "fc_%s_%s", SHAPES[F.shape], COLORS[F.color]); break;
    case M_DIVERSO: snprintf(id, sizeof(id), "fc_diverso"); break;
    case M_NON_FORMA: snprintf(id, sizeof(id), "fc_non_%s", SHAPES[F.shape]); break;
    default: snprintf(id, sizeof(id), "fc_nonc_%s", COLORS[F.color]); break;
    }
    say(id);
}

static void hover(quiz_t *q, int i)
{
    char id[48];
    snprintf(id, sizeof(id), "fc_n_%s_%s", SHAPES[F.opt[i].shape], COLORS[F.opt[i].color]);
    say(id);
}

static void right(quiz_t *q)
{
    static const char *MAGO[] = {"fc_mago_1", "fc_mago_2", "fc_mago_3"};
    F.fly_t = 0;
    if (!q->duel)
        say_then(MAGO[rng_range(0, 2)]);
}

static void tick(quiz_t *q)
{
    F.bubble_t++;
    if (F.fly_t >= 0 && ++F.fly_t == 26) { /* into the cauldron */
        F.potion = F.opt[q->correct].color;
        sfx("bloop");
    }
}

static bool hint(quiz_t *q)
{
    char id[48];
    bool shape_hint = F.mode == M_FORMA || F.mode == M_ENTRAMBI;
    bool color_hint = F.mode == M_COLORE || F.mode == M_ENTRAMBI;
    if (F.mode >= M_DIVERSO) {
        if (q->hint_step == 0) {
            say("fc_h_non"); /* "trova quello che è diverso dagli altri due" */
            return false;
        }
        return true;
    }
    if (q->hint_step == 0 && shape_hint) {
        snprintf(id, sizeof(id), "fc_h_%s", SHAPES[F.shape]);
        say(id);
        return false;
    }
    if (q->hint_step <= 1 && color_hint && !(q->hint_step == 1 && !shape_hint)) {
        snprintf(id, sizeof(id), "fc_hc_%s", COLORS[F.color]);
        say(id);
        q->hint_step = 1;
        return false;
    }
    ask(q);
    return true;
}

/* ------------------------------------------------------------------ drawing */

static void draw_shape(int shape, int color, int cx, int cy, bool off)
{
    char name[32];
    uint16_t base = off ? rgb565(0xd6, 0xd0, 0xe0) : col(color, 0);
    uint16_t light = off ? rgb565(0xe8, 0xe4, 0xf0) : col(color, 1);
    uint16_t dark = off ? rgb565(0xb0, 0xa4, 0xc8) : col(color, 2);
    snprintf(name, sizeof(name), "fs_%s", SHAPES[shape]);
    const sprite_t *m = gfx_sprite(name);
    int x = cx - m->w / 2, y = cy - m->h / 2;
    snprintf(name, sizeof(name), "fs_%s_o", SHAPES[shape]);
    gfx_blit_solid(gfx_sprite(name), x, y, off ? rgb565(0xb0, 0xa4, 0xc8) : COL_INK, 0);
    gfx_blit_solid(m, x, y, base, 0);
    snprintf(name, sizeof(name), "fs_%s_l", SHAPES[shape]);
    gfx_blit_solid(gfx_sprite(name), x, y, light, 0);
    snprintf(name, sizeof(name), "fs_%s_d", SHAPES[shape]);
    gfx_blit_solid(gfx_sprite(name), x, y, dark, 0);
}

static void draw(quiz_t *q)
{
    int ccx = q->duel ? q->panel_cx : q->panel_cx + 40, cbot = 160;
    if (!q->duel) { /* the wizard next to his cauldron, talking while the voice speaks */
        int lv = voice_busy() ? voice_level() : 0; /* 0.13: his mouth and his body with the voice */
        const sprite_t *m = gfx_sprite("mago");
        actor_draw(lv > 80 ? "mago_p" : "mago", q->panel_cx - 100 + m->w / 2, cbot, 1, 0, lv, 0, 0);
    }
    const sprite_t *cal = gfx_sprite("calderone");
    int cx0 = ccx - cal->w / 2, cy0 = cbot - cal->h;
    gfx_blit(cal, cx0, cy0, 0);
    if (F.potion >= 0) { /* the potion, in the colour of the last shape, and its bubbles */
        const sprite_t *pz = gfx_sprite("pozione");
        gfx_blit_solid(pz, ccx - pz->w / 2, cy0 + 10 - pz->h / 2, col(F.potion, 0), 0);
        const sprite_t *b = gfx_sprite("bolla");
        for (int i = 0; i < 3; i++) {
            int t = (F.bubble_t + i * 23) % 70;
            gfx_blit_solid(b, ccx - 17 + i * 14 + ((t / 10) & 1), cy0 + 4 - t / 2, col(F.potion, 1), 0);
        }
    }
    if (F.fly_t >= 0 && F.fly_t < 26) { /* the chosen shape jumps into the cauldron */
        int sx = quiz_card_cx(q->correct), sy = quiz_card_cy(), tx = ccx, ty = cy0 + 8, k = F.fly_t;
        int x = sx + (tx - sx) * k / 26, y = sy + (ty - sy) * k / 26 - 200 * k * (26 - k) / (26 * 26);
        draw_shape(F.opt[q->correct].shape, F.opt[q->correct].color, x, y, false);
    }
    if (F.fly_t == 26)
        fx_sparkles(ccx, cy0 + 6, 16, 12);
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (F.fly_t >= 0 && i == q->correct && F.fly_t < 26)
        return; /* flying */
    draw_shape(F.opt[i].shape, F.opt[i].color, cx, cy, style == CARD_OFF);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    snprintf(buf, n, "%s_%s", SHAPES[F.opt[i].shape], COLORS[F.opt[i].color]);
}

static const char *subject(quiz_t *q)
{
    static const char *MODES[] = {"forma", "colore", "entrambi", "diverso", "non_forma", "non_colore"};
    static char s[48];
    if (F.mode == M_NON_FORMA) /* "quale non è un cerchio?" */
        snprintf(s, sizeof(s), "%s:%s", MODES[F.mode], SHAPES[F.shape]);
    else if (F.mode == M_NON_COLORE) /* "quale non è rosso?" */
        snprintf(s, sizeof(s), "%s:%s", MODES[F.mode], COLORS[F.color]);
    else /* the card to find */
        snprintf(s, sizeof(s), "%s:%s_%s", MODES[F.mode], SHAPES[F.shape], COLORS[F.color]);
    return s;
}

const quiz_def_t QUIZ_FORME = {
    .name = "forme",
    .game = GAME_FORME,
    .bg = "bg_laboratorio",
    .intro = "fc_intro",
    .card = QUIZ_CARD_PICTURE,
    .setup = setup,
    .prepare = prepare,
    .tick = tick,
    .ask = ask,
    .hover = hover,
    .right = right,
    .hint = hint,
    .draw = draw,
    .intro_draw = draw, /* the wizard and his empty cauldron while the intro names him */
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void)
{
    /* the intro comes before the first setup(): nothing flying, a clear cauldron
       (a zeroed fly_t would "bloop" a potion into it during the intro) */
    F.fly_t = -1;
    F.potion = -1;
    quiz_enter(&QUIZ_FORME);
}

const scene_t SCENE_FORME = {enter, quiz_update, quiz_draw};
