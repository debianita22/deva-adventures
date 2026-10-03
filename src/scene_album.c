/* Deva's Awesome Adventures - the album of treasures: pages to leaf through
 * with the cross. The tale (gems and the friends set free; each adventure
 * has its own page once it has begun), the games (their stars), the make-up
 * won, the stickers (two pages, once the first one is won) and all the stars
 * collected. Things still to be won are dark shapes. On the page of an
 * adventure told to the end, the red button tells it again (0.11.0: its
 * beginning and its ending, nothing changes in the game). The yellow button
 * goes back.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "story.h"
#include "ui.h"

enum {
    PG_STORIA, PG_STORIA2, PG_STORIA3, PG_STORIA4, PG_GIOCHI, PG_GIOCHI2, PG_TRUCCHI, PG_ADESIVI, PG_ADESIVI2,
    PG_STELLE, PG_COUNT
};

static const char *TITLES[PG_COUNT] = {"LA STORIA", "LA STORIA 2", "LA STORIA 3", "LA STORIA 4", "I GIOCHI",
                                       "I GIOCHI 2", "I TRUCCHI", "GLI ADESIVI", "GLI ADESIVI 2", "LE STELLE"};
static const char *VOICES[PG_COUNT] = {"alb_storia", "alb_storia2", "alb_storia3", "alb_storia4", "alb_giochi",
                                       "alb_giochi", "alb_trucchi", "alb_adesivi", "alb_adesivi", "alb_stelle"};
#define GAMES_PAGE 9 /* 18 games: two pages of 5 + 4 */
#define GAMES_ROW 5

#define ST_COLS 7
#define ST_PAGE (STICKER_COUNT / 2) /* 21 stickers a page, 7 x 3 */

static struct {
    int pages[PG_COUNT], n; /* the pages shown: each tale once it has begun */
    int page, t, flip;      /* page = index in pages; flip: frames of the page turn, sign = direction */
    int lost;               /* frames without turning a page (0.14: then "to go back, the yellow button") */
} A;

/* the page to open on: the one of the adventure told again, when it is over */
static int s_return_page = -1;

/* the adventure of a page of the tale, or -1 */
static int page_arc(int pg) { return pg >= PG_STORIA && pg <= PG_STORIA4 ? pg - PG_STORIA : -1; }

/* that adventure can be told again: it has been told to the end */
static bool can_replay(int pg)
{
    int arc = page_arc(pg);
    return arc >= 0 && G.cfg.story && story_arc_done(arc);
}

/* 0.9.0: what she has won is alive - a little hop that runs along the page like
   a wave, and twinkles here and there (the places drawn this frame, for update) */
static int s_lit_x[48], s_lit_y[48], s_nlit;

static int hop(int i) { return (unsigned)(A.t / 6 + 400 - i * 3) % 40u < 3 ? -1 : 0; }

static void lit_at(int x, int y)
{
    if (s_nlit < (int)ARRAY_LEN(s_lit_x)) {
        s_lit_x[s_nlit] = x;
        s_lit_y[s_nlit] = y;
        s_nlit++;
    }
}

static void show_page(int p)
{
    A.page = clampi(p, 0, A.n - 1);
    A.t = 0;
    say(VOICES[A.pages[A.page]]);
    if (can_replay(A.pages[A.page]))
        say_then("alb_rivedi"); /* "premi il bottone rosso, e rivediamo la storia!" */
    BOT("album page=%d\n", A.pages[A.page]);
}

static void enter(void)
{
    A.flip = 0;
    A.n = 0;
    int open_at = 0;
    for (int p = 0; p < PG_COUNT; p++) {
        if (page_arc(p) > 0 && G.prog.arc < page_arc(p))
            continue;
        if ((p == PG_ADESIVI || p == PG_ADESIVI2) && !G.prog.stickers)
            continue;
        if (p == s_return_page)
            open_at = A.n;
        A.pages[A.n++] = p;
    }
    s_return_page = -1;
    hero_init(&G.hero, 160, 236);
    music_play(snd_find(SND_MUSIC, "palco"));
    show_page(open_at);
}

static void update(void)
{
    A.t++;
    if (A.t % 14 == 0 && s_nlit > 0 && !A.flip) {
        int k = rng_range(0, s_nlit - 1);
        int dx, dy;
        rng_pair(-12, 12, -12, 8, &dx, &dy);
        fx_twinkle(s_lit_x[k] + dx, s_lit_y[k] + dy, 0);
    }
    if (A.flip)
        A.flip += A.flip > 0 ? -1 : 1;
    /* lost in the album? how to go back: after 20 s without turning a page, then every 30 s */
    if (!voice_busy() && ++A.lost == 20 * FPS) {
        say("torna_giallo");
        A.lost = -10 * FPS;
    }
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir && A.page + dir >= 0 && A.page + dir < A.n) {
        A.lost = 0;
        sfx("whoosh");
        A.flip = dir * 10;
        show_page(A.page + dir);
    } else if (btn_pressed(BTN_A) && can_replay(A.pages[A.page])) { /* tell it again */
        sfx("star");
        s_return_page = A.pages[A.page];
        racconto_replay(page_arc(A.pages[A.page]), SC_ALBUM);
    } else if (btn_pressed(BTN_A) || btn_pressed(BTN_Y)) {
        say(VOICES[A.pages[A.page]]);
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_START)) {
        sfx("blip");
        G.title_sel = 0; /* back on "Gioca": the red button plays (0.14: else it came back here, round and round) */
        game_goto(SC_TITLE);
    }
}

static void slot_card(int x, int y, int w, int h, bool lit)
{
    gfx_round_rect(x, y, w, h, lit ? COL_LILAC_L : COL_GREY, COL_INK);
}

static void draw_storia(int ox, int arc)
{
    /* five places and their gems (stars), and the villain: set free = in colour */
    const arc_t *a = &ARCS[arc];
    int freed = arc < G.prog.arc ? CH_COUNT : (arc == G.prog.arc ? G.prog.chapter : 0);
    bool villain = arc < G.prog.arc || (freed >= CH_COUNT && (G.prog.seen & ((uint32_t)SEEN_FINALE << (8 * arc))));
    char name[40];
    for (int i = 0; i <= CH_COUNT; i++) {
        int cx = ox + 30 + i * 52, y = 66;
        bool lit = i < CH_COUNT ? i < freed : villain;
        slot_card(cx - 24, y, 48, 48, lit);
        const char *foe = a->ch[i < CH_COUNT ? i : CH_VILLAIN].foe;
        snprintf(name, sizeof(name), lit ? "amico_%s" : "menu_sfida_%s", foe);
        const sprite_t *s = gfx_sprite(name);
        if (lit) {
            gfx_blit(s, cx - 22, y + 2 + hop(i), 0);
            lit_at(cx, y + 24);
        } else {
            ui_silhouette(s, cx - 22, y + 2);
        }
        if (i < CH_COUNT) {
            const sprite_t *g = gfx_sprite(a->ch[i].gem);
            if (lit)
                gfx_blit(g, cx - g->w / 2, y + 56, 0);
            else
                ui_silhouette(g, cx - g->w / 2, y + 56);
        } else {
            gfx_blit(gfx_sprite(lit ? "star_on" : "star_off"), cx - 8, y + 57, 0);
        }
    }
    char line[64];
    static const char *const WHAT[ARC_COUNT] = {"Gemme", "Stelle", "Note", "Chiavi"};
    snprintf(line, sizeof(line), "%s ritrovate: %d su %d", WHAT[arc], freed < CH_COUNT ? freed : CH_COUNT, CH_COUNT);
    gfx_text_shadow(line, ox + 160, 150, COL_CREAM, COL_DEEP, ALIGN_CENTER);
    if (G.cfg.story && story_arc_done(arc)) { /* the red button: tell it again */
        const sprite_t *b = gfx_sprite("btn_a");
        int bob = (A.t / 20) & 1;
        gfx_blit(b, ox + 160 - 52, 172 - bob, 0);
        gfx_text_shadow("Rivedi la storia", ox + 160 - 52 + b->w + 6, 176, COL_CREAM, COL_DEEP, ALIGN_LEFT);
    }
}

static void draw_giochi(int ox, int first)
{
    for (int g = first; g < first + GAMES_PAGE && g < GAME_COUNT; g++) {
        int pos = g - first, row = pos / GAMES_ROW;
        int in_row = imin(GAMES_ROW, imin(GAME_COUNT, first + GAMES_PAGE) - first - row * GAMES_ROW);
        int cx = ox + 160 + ((pos % GAMES_ROW) * 2 - (in_row - 1)) * 30, y = 48 + row * 80;
        bool open = game_is_unlocked(&G.prog, &G.cfg, g);
        slot_card(cx - 24, y, 48, 48, open);
        const sprite_t *s = gfx_sprite(GAME_ICONS[g]);
        if (open) {
            gfx_blit(s, cx - 22, y + 2 + hop(g), 0);
            lit_at(cx, y + 24);
        } else {
            ui_silhouette(s, cx - 22, y + 2);
        }
        int lvl = clampi(G.prog.level[g], 1, 5);
        gfx_round_rect(cx - 29, y + 49, 58, 13, COL_DEEP, COL_INK);
        for (int k = 0; k < 5; k++)
            gfx_blit(gfx_sprite(open && k < lvl ? "lstar_on" : "lstar_off"), cx - 27 + k * 11, y + 50, 0);
    }
}

static void draw_trucchi(int ox)
{
    char name[40];
    for (int i = 0; i < ITEM_COUNT; i++) {
        int row = i < 7 ? 0 : 1, col = row ? i - 7 : i;
        int n = row ? ITEM_COUNT - 7 : 7;
        int cx = ox + 160 + (col * 2 - (n - 1)) * 22, y = 62 + row * 56;
        bool own = (G.prog.owned >> i) & 1u;
        slot_card(cx - 21, y, 42, 42, own);
        snprintf(name, sizeof(name), "rw_%s", ITEMS[i].id);
        const sprite_t *s = gfx_sprite(name);
        if (own) {
            gfx_blit(s, cx - 18, y + 3 + hop(i), 0);
            lit_at(cx, y + 21);
        } else {
            ui_silhouette(s, cx - 18, y + 3);
        }
    }
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        n += (G.prog.owned >> i) & 1u;
    char line[64];
    snprintf(line, sizeof(line), "Trucchi vinti: %d su %d", n, ITEM_COUNT);
    gfx_text_shadow(line, ox + 160, 176, COL_CREAM, COL_DEEP, ALIGN_CENTER);
}

static void draw_adesivi(int ox, int first)
{
    char name[40];
    for (int k = 0; k < ST_PAGE && first + k < STICKER_COUNT; k++) {
        int i = first + k, cx = ox + 160 + (k % ST_COLS - ST_COLS / 2) * 44, y = 42 + (k / ST_COLS) * 52;
        bool own = (G.prog.stickers >> i) & 1u;
        slot_card(cx - 21, y, 42, 42, own);
        snprintf(name, sizeof(name), "pic_%s", STICKERS[i]);
        const sprite_t *s = gfx_sprite(name);
        if (own) {
            ui_sticker(s, cx - s->w / 2, y + 21 - s->h / 2 + hop(k));
            lit_at(cx, y + 21);
        } else
            ui_silhouette(s, cx - s->w / 2, y + 21 - s->h / 2);
    }
    int n = 0;
    for (int i = 0; i < STICKER_COUNT; i++)
        n += (G.prog.stickers >> i) & 1u;
    char line[64];
    snprintf(line, sizeof(line), "Adesivi vinti: %d su %d", n, STICKER_COUNT);
    gfx_text_shadow(line, ox + 160, 202, COL_CREAM, COL_DEEP, ALIGN_CENTER);
}

static void draw_stelle(int ox)
{
    const sprite_t *st = gfx_sprite("star_on");
    int bob = ((A.t / 20) & 1);
    gfx_blit_scaled(st, ox + 160 - 32, 50 + bob, 4, 0);
    lit_at(ox + 160 - 26, 60);
    lit_at(ox + 160 + 26, 60);
    lit_at(ox + 160, 110);
    gfx_number_big(G.prog.stars_total, ox + 160, 142, NUM_SELECTED);
    char line[80];
    long min = (G.prog.play_seconds + G.play_frames / FPS) / 60;
    snprintf(line, sizeof(line), "Round giocati: %d   Volte che hai giocato: %d", G.prog.rounds_total,
             G.prog.sessions);
    gfx_text_shadow(line, ox + 160, 170, COL_CREAM, COL_DEEP, ALIGN_CENTER);
    if (min >= 60)
        snprintf(line, sizeof(line), "Tempo di gioco: %ld h %ld min", min / 60, min % 60);
    else
        snprintf(line, sizeof(line), "Tempo di gioco: %ld min", min);
    gfx_text_shadow(line, ox + 160, 182, COL_CREAM, COL_DEEP, ALIGN_CENTER);
}

static void draw(void)
{
    s_nlit = 0;
    ui_backdrop("bg_palco", 110);
    int ox = A.flip * 12; /* the new page slides in */
    int pg = A.pages[A.page];
    ui_title(TITLES[pg], 20);
    switch (pg) {
    case PG_STORIA: draw_storia(ox, ARC_GRIGIO); break;
    case PG_STORIA2: draw_storia(ox, ARC_NOTTE); break;
    case PG_STORIA3: draw_storia(ox, ARC_MUSICA); break;
    case PG_STORIA4: draw_storia(ox, ARC_GIOCHI); break;
    case PG_GIOCHI: draw_giochi(ox, 0); break;
    case PG_GIOCHI2: draw_giochi(ox, GAMES_PAGE); break;
    case PG_TRUCCHI: draw_trucchi(ox); break;
    case PG_ADESIVI: draw_adesivi(ox, 0); break;
    case PG_ADESIVI2: draw_adesivi(ox, ST_PAGE); break;
    default: draw_stelle(ox); break;
    }
    /* page dots and arrows */
    for (int p = 0; p < A.n; p++)
        gfx_blit(gfx_sprite(p == A.page ? "lstar_on" : "lstar_off"), 160 - A.n * 7 + p * 14, 224, 0);
    if (A.page > 0 && ((G.frame / 20) & 1))
        gfx_blit(gfx_sprite("arrow_left"), 160 - A.n * 7 - 30, 217, 0);
    if (A.page < A.n - 1 && ((G.frame / 20) & 1))
        gfx_blit(gfx_sprite("arrow_right"), 160 + A.n * 7 + 6, 217, 0);
    ui_back_hint();
    fx_draw();
}

const scene_t SCENE_ALBUM = {enter, update, draw};
