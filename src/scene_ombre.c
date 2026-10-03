/* Deva's Awesome Adventures - "Le ombre" (0.10.0): whose shadow is this?
 *
 * In the shadow theatre a lamp throws the shadow of a thing on the white
 * sheet; she picks, among three picture cards, the thing that makes it. The
 * other way round, the thing is shown in colour and the cards are shadows.
 * It trains looking at shapes: outlines, proportions, the details that make
 * a cat a cat and not a dog (the first step towards telling letters apart).
 *
 * How alike two shadows are is measured once, on the pictures themselves:
 * each outline is scaled into a 24x24 box (proportions kept) and two outlines
 * are as alike as the part of them that overlaps.
 * Levels: 1 the two other cards very different from the right one; 2 any;
 * 3 lookalike shadows; 4 the thing in colour, pick its shadow; 5 both, with
 * lookalikes, and a little cloud sometimes hides a piece of the shadow.
 * Guided help: "guarda bene la forma", and the colour comes up from below
 * over half the shadow; then the right card lights up.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

/* the pictures of the word games that have their voice (w_<id>) */
static const char *const PICS[] = {
    "stella",  "cuore",    "farfalla",  "fiore",   "diamante", "corona",   "scarpetta", "rossetto",  "fiocco",
    "microfono", "pennello", "smalto",  "palla",   "mela",     "luna",     "sole",      "pesce",     "casa",
    "torta",   "banana",   "gelato",    "palloncino", "ape",   "uva",      "uovo",      "ombrello",  "isola",
    "elefante", "cane",    "pane",      "gatto",   "piatto",   "nave",     "chiave",    "porta",     "vela",
    "candela", "padella",  "caramella", "letto",   "rana",     "campana",  "pulcino",
};
#define NPICS ((int)ARRAY_LEN(PICS))
#define GRID 24
#define HISTORY 5

enum { M_OMBRA, M_COLORE }; /* shadow shown: pick the thing; thing shown: pick the shadow */

static struct {
    bool measured;
    uint8_t sim[NPICS][NPICS]; /* 0..255: how alike two shadows are */
    int mode;
    int item, opt[3];
    int history[HISTORY];
    bool cloud;                   /* a little cloud covers a piece of the shadow */
    int cloud_x, cloud_y;
    int reveal;                   /* guided help: colour rising over the shadow, 0..100 % */
} O;

static const sprite_t *pic(int i)
{
    char name[32];
    snprintf(name, sizeof(name), "pic_%s", PICS[i]);
    return gfx_sprite(name);
}

/* the outline of a picture in a 24x24 box, proportions kept, centred */
static void outline(int i, uint8_t out[GRID][GRID])
{
    const sprite_t *s = pic(i);
    memset(out, 0, GRID * GRID);
    int x0 = s->w, y0 = s->h, x1 = -1, y1 = -1;
    for (int y = 0; y < s->h; y++)
        for (int x = 0; x < s->w; x++)
            if (gfx_opaque(s, x, y)) {
                x0 = x < x0 ? x : x0;
                y0 = y < y0 ? y : y0;
                x1 = x > x1 ? x : x1;
                y1 = y > y1 ? y : y1;
            }
    if (x1 < 0)
        return;
    int w = x1 - x0 + 1, h = y1 - y0 + 1, side = w > h ? w : h;
    int ox = (side - w) / 2, oy = (side - h) / 2;
    for (int gy = 0; gy < GRID; gy++)
        for (int gx = 0; gx < GRID; gx++) {
            int sx = gx * side / GRID - ox, sy = gy * side / GRID - oy;
            out[gy][gx] = sx >= 0 && sy >= 0 && sx < w && sy < h && gfx_opaque(s, x0 + sx, y0 + sy);
        }
}

static void measure(void)
{
    static uint8_t shape[NPICS][GRID][GRID];
    O.measured = true;
    for (int i = 0; i < NPICS; i++)
        outline(i, shape[i]);
    for (int a = 0; a < NPICS; a++)
        for (int b = a; b < NPICS; b++) {
            int both = 0, any = 0;
            for (int y = 0; y < GRID; y++)
                for (int x = 0; x < GRID; x++) {
                    both += shape[a][y][x] & shape[b][y][x];
                    any += shape[a][y][x] | shape[b][y][x];
                }
            uint8_t v = (uint8_t)(any ? both * 255 / any : 0);
            O.sim[a][b] = O.sim[b][a] = v;
        }
}

static bool recent(int i)
{
    for (int k = 0; k < HISTORY; k++)
        if (O.history[k] == i)
            return true;
    return false;
}

/* the others sorted by how alike their shadow is to the item's, most alike first */
static int by_likeness(int *out)
{
    int n = 0;
    for (int i = 0; i < NPICS; i++)
        if (i != O.item)
            out[n++] = i;
    for (int a = 1; a < n; a++) { /* insertion sort, n = 42 */
        int v = out[a], b = a - 1;
        while (b >= 0 && O.sim[O.item][out[b]] < O.sim[O.item][v]) {
            out[b + 1] = out[b];
            b--;
        }
        out[b + 1] = v;
    }
    return n;
}

static void setup(quiz_t *q, int level)
{
    if (!O.measured)
        measure();
    if (q->question == 0)
        for (int k = 0; k < HISTORY; k++)
            O.history[k] = -1;
    O.mode = level <= 3 ? M_OMBRA : (level == 4 ? M_COLORE : (rng_range(0, 1) ? M_COLORE : M_OMBRA));
    do
        O.item = rng_range(0, NPICS - 1);
    while (recent(O.item));
    memmove(&O.history[1], &O.history[0], sizeof(int) * (HISTORY - 1));
    O.history[0] = O.item;
    int order[NPICS], n = by_likeness(order), lo = 0, hi = n; /* the range the two others come from */
    if (level == 1) {
        lo = n - 12; /* the most different ones */
    } else if (level == 2 || level == 4) {
        lo = 6;      /* anything but the closest ones */
    } else {
        hi = 6;      /* the six most alike */
    }
    int d0 = order[rng_range(lo, hi - 1)], d1;
    do
        d1 = order[rng_range(lo, hi - 1)];
    while (d1 == d0);
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        O.opt[i] = i == q->correct ? O.item : (k++ == 0 ? d0 : d1);
    q->card = QUIZ_CARD_PICTURE;
    O.cloud = level >= 5 && O.mode == M_OMBRA && rng_range(0, 1);
    O.cloud_x = rng_range(0, 1) ? 1 : -1;
    O.cloud_y = rng_range(0, 1) ? 1 : -1;
    O.reveal = 0;
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("whoosh"); /* the lamp lights up */
    return q->t > 24;
}

static void tick(quiz_t *q)
{
    if (O.reveal > 0 && O.reveal < 50)
        O.reveal++;
}

static void ask(quiz_t *q)
{
    say(O.mode == M_OMBRA ? "omb_chi" : "omb_quale");
    if (O.cloud && q->question < 2)
        say_then("omb_nuvola");
}

static void say_item(int i, bool queue)
{
    char id[32];
    snprintf(id, sizeof(id), "w_%s", PICS[i]);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void hover(quiz_t *q, int i)
{
    if (O.mode == M_OMBRA)
        say_item(O.opt[i], false);
    /* shadow cards: their name would give the answer away */
}

static void right(quiz_t *q) { say_item(O.item, true); }

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("omb_guarda");
        if (O.mode == M_OMBRA)
            O.reveal = 1; /* the colour rises over the lower half of the shadow */
        return false;
    }
    say_item(O.item, false);
    return true;
}

#define SHADOW rgb565(0x2e, 0x1f, 0x42)

static void shadow(const sprite_t *s, int x, int y, int k)
{
    gfx_set_tint(SHADOW, 256);
    gfx_blit_scaled(s, x, y, k, 0);
    gfx_set_tint(0, 0);
}

static void draw(quiz_t *q)
{
    int cx = q->panel_cx, cy = q->panel_cy + 2;
    const sprite_t *s = pic(O.item);
    int x = cx - s->w, y = cy - s->h - 4;
    bool solved = q->state == Q_RIGHT;
    if (O.mode == M_COLORE) {
        gfx_blit_scaled(s, x, y, 2, 0); /* the thing, in colour */
        return;
    }
    if (solved && q->t > 12) { /* the shadow gets its colours */
        gfx_blit_scaled(s, x, y, 2, 0);
        return;
    }
    shadow(s, x, y, 2);
    if (O.reveal > 0) { /* guided help: the colour rises from below */
        int top = y + s->h * 2 - s->h * 2 * O.reveal / 100;
        gfx_set_clip(0, top, SCREEN_W, SCREEN_H);
        gfx_blit_scaled(s, x, y, 2, 0);
        gfx_reset_state();
    }
    if (O.cloud && !solved) { /* a sleepy little cloud on one corner */
        const sprite_t *c = gfx_sprite("puff_1");
        int px = cx + O.cloud_x * 16 - c->w, py = cy + O.cloud_y * 14 - 4 - c->h;
        gfx_blit_scaled(c, px, py, 2, 0);
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    const sprite_t *s = pic(O.opt[i]);
    if (O.mode == M_COLORE) {
        if (style == CARD_OFF)
            gfx_set_tint(rgb565(0x8a, 0x80, 0xa0), 256);
        else
            gfx_set_tint(SHADOW, 256);
        gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
        gfx_set_tint(0, 0);
        return;
    }
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%s", PICS[O.opt[i]]); }

static const char *subject(quiz_t *q)
{
    static char s[48];
    snprintf(s, sizeof(s), "%s:%s%s", O.mode == M_OMBRA ? "ombra" : "colore", PICS[O.item], O.cloud ? ":nuvola" : "");
    return s;
}

const quiz_def_t QUIZ_OMBRE = {
    .name = "ombre",
    .game = GAME_OMBRE,
    .bg = "bg_ombre",
    .intro = "omb_intro",
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

static void enter(void) { quiz_enter(&QUIZ_OMBRE); }

const scene_t SCENE_OMBRE = {enter, quiz_update, quiz_draw};
