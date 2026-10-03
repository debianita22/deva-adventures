/* Deva's Awesome Adventures - "Memory del camerino": pairs of make-up cards
 * face down in the dressing room; she moves with the black cross, turns two
 * cards with the red button and keeps the pairs she finds (each says the
 * name of the make-up). Levels: 3x2, 4x2, 4x3, 4x3 with look-alike make-up
 * (the same thing in different colours), 4x4 cards.
 * Too many misses: "Sbirciamo!" - all cards show for a moment (guided help).
 * One grid is one round; each pair found is a star.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "ui.h"

#define CARD 44
#define GAP 4
#define MAXC 16
#define FLIP 12        /* frames of the turning animation */
#define SHOW_MISS 55   /* frames two different cards stay up */
#define PEEK 110       /* frames of the "sbirciamo" help */
#define AREA_CX 214    /* centre of the grid, right of the mirror */
#define AREA_CY 118

/* mirror glass in bg_camerino (see tools/art/backgrounds.py) */
#define GLASS_X0 19
#define GLASS_Y0 33
#define GLASS_X1 102
#define GLASS_Y1 171

static const struct {
    int cols, rows;
} LEVELS[5] = {{3, 2}, {4, 2}, {4, 3}, {4, 3}, {4, 4}};
#define LOOKALIKE_LEVEL 4

enum { MM_INTRO, MM_PLAY, MM_MISS, MM_PEEK, MM_DONE };

static struct {
    int state, t, level;
    int cols, rows, n, pairs;
    int item[MAXC];        /* make-up item per card */
    bool up[MAXC], found[MAXC];
    int flip_t[MAXC];      /* > 0 while turning */
    int cur;               /* cursor */
    int first, second;     /* cards turned in this try, -1 = none */
    int found_pairs, misses;
    bool peeked;
    int quiet;
} M;

static int card_x(int i) { return AREA_CX - (M.cols * (CARD + GAP) - GAP) / 2 + (i % M.cols) * (CARD + GAP); }
static int card_y(int i) { return AREA_CY - (M.rows * (CARD + GAP) - GAP) / 2 + (i / M.cols) * (CARD + GAP); }

static int same_kind(int item) /* items in the same slot, this one included */
{
    int n = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        n += ITEMS[i].slot == ITEMS[item].slot;
    return n;
}

static void deal(void)
{
    M.level = clampi(G.prog.level[GAME_MEMORY], 1, 5);
    M.cols = LEVELS[M.level - 1].cols;
    M.rows = LEVELS[M.level - 1].rows;
    M.n = M.cols * M.rows;
    M.pairs = M.n / 2;
    /* the make-up she has won first, then the others; at level 4 only the kinds
       that come in more than one colour (ombretto rosa, ombretto viola...) */
    int pool[32], np = 0;
    for (int pass = 0; pass < 2; pass++) {
        int start = np;
        for (int i = 0; i < ITEM_COUNT; i++) {
            if (M.level == LOOKALIKE_LEVEL && same_kind(i) < 2)
                continue;
            if (((G.prog.owned >> i) & 1u) == (pass == 0))
                pool[np++] = i;
        }
        for (int i = np - 1; i > start; i--) { /* shuffle within the pass */
            int j = start + rng_range(0, i - start), t = pool[i];
            pool[i] = pool[j];
            pool[j] = t;
        }
    }
    for (int p = 0; p < M.pairs; p++)
        M.item[2 * p] = M.item[2 * p + 1] = pool[p];
    for (int i = M.n - 1; i > 0; i--) {
        int j = rng_range(0, i), t = M.item[i];
        M.item[i] = M.item[j];
        M.item[j] = t;
    }
    memset(M.up, 0, sizeof(M.up));
    memset(M.found, 0, sizeof(M.found));
    memset(M.flip_t, 0, sizeof(M.flip_t));
    M.cur = 0;
    M.first = M.second = -1;
    M.found_pairs = M.misses = 0;
    M.peeked = false;
    char layout[64];
    int pos = 0;
    for (int i = 0; i < M.n && pos < (int)sizeof(layout) - 4; i++)
        pos += snprintf(layout + pos, sizeof(layout) - pos, "%s%d", i ? "," : "", M.item[i]);
    BOT("memory grid=%dx%d level=%d layout=%s\n", M.cols, M.rows, M.level, layout);
}

static void enter(void)
{
    memset(&M, 0, sizeof(M));
    hero_init(&G.hero, 60, 212);
    music_play(snd_find(SND_MUSIC, "palco"));
    deal();
    if (!G.intro_done[GAME_MEMORY]) {
        G.intro_done[GAME_MEMORY] = true;
        say("memory_intro");
        hero_play(&G.hero, HA_WAVE, 90);
        M.state = MM_INTRO;
    } else {
        M.state = MM_PLAY;
        game_question_ready(); /* the board: timed as a whole */
        BOT("memory_ready cursor=%d\n", M.cur);
    }
}

static void turn(int i, bool up)
{
    M.up[i] = up;
    M.flip_t[i] = FLIP;
}

static void say_item(int item, bool queue)
{
    char id[40];
    snprintf(id, sizeof(id), "t_%s", ITEMS[item].id);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void move_cursor(int dx, int dy)
{
    int c = M.cur % M.cols + dx, r = M.cur / M.cols + dy;
    if (c < 0 || c >= M.cols || r < 0 || r >= M.rows)
        return;
    M.cur = r * M.cols + c;
    sfx("blip");
    BOT("memory cursor=%d\n", M.cur);
}

static void finish(void)
{
    bool good = M.misses <= M.pairs, assisted = M.peeked;
    char target[16], chosen[16], subject[24];
    snprintf(subject, sizeof(subject), "memory:%dx%d", M.cols, M.rows);
    snprintf(target, sizeof(target), "coppie=%d", M.pairs);
    snprintf(chosen, sizeof(chosen), "errori=%d", M.misses);
    game_log_answer("memory", M.level, subject, target, chosen, good, 1);
    BOT("memory_done misses=%d peeked=%d\n", M.misses, M.peeked);
    bool up = game_level_result(GAME_MEMORY, good && !assisted, assisted);
    sfx("fanfare");
    say("memory_fatto");
    hero_play(&G.hero, HA_DANCE, 160);
    fx_confetti(AREA_CX, AREA_CY, 50);
    fx_burst(AREA_CX, AREA_CY, 16);
    fx_hearts(G.hero.x + 4, G.hero.y - 70, 4);
    if (up)
        fx_starfall(26); /* a shower of stars for the new level */
    M.state = MM_DONE;
    M.t = 0;
}

static void pick(void)
{
    int i = M.cur;
    if (M.found[i] || M.up[i])
        return;
    turn(i, true);
    sfx("pop");
    BOT("memory_flip idx=%d item=%d\n", i, M.item[i]);
    if (M.first < 0) {
        M.first = i;
        return;
    }
    M.second = i;
    if (M.item[M.first] == M.item[i]) { /* a pair: it stays, with its name */
        M.found[M.first] = M.found[i] = true;
        M.found_pairs++;
        G.prog.stars_total++;
        sfx("ding");
        say_item(M.item[i], false);
        hero_play(&G.hero, HA_DANCE, 60);
        fx_sparkles(card_x(i) + CARD / 2, card_y(i) + CARD / 2, 14, 10);
        fx_sparkles(card_x(M.first) + CARD / 2, card_y(M.first) + CARD / 2, 14, 10);
        fx_burst(card_x(i) + CARD / 2, card_y(i) + CARD / 2, 8);
        fx_burst(card_x(M.first) + CARD / 2, card_y(M.first) + CARD / 2, 8);
        fx_hearts(G.hero.x + 4, G.hero.y - 70, 1);
        game_star_fly(card_x(i) + CARD / 2, card_y(i) + CARD / 2, AREA_CX, M.found_pairs - 1, M.pairs);
        BOT("memory_match item=%d\n", M.item[i]);
        M.first = M.second = -1;
        if (M.found_pairs == M.pairs)
            finish();
        return;
    }
    M.misses++;
    BOT("memory_miss misses=%d\n", M.misses);
    M.state = MM_MISS;
    M.t = 0;
}

static void update(void)
{
    M.t++;
    for (int i = 0; i < M.n; i++)
        if (M.flip_t[i] > 0)
            M.flip_t[i]--;
    if (M.found_pairs > 0 && M.t % 25 == 0) { /* the pairs found shine now and then */
        int i = rng_range(0, M.n - 1);
        if (M.found[i]) {
            int dx, dy;
            rng_pair(6, CARD - 6, 4, 14, &dx, &dy);
            fx_twinkle(card_x(i) + dx, card_y(i) + dy, 0);
        }
    }
    switch (M.state) {
    case MM_INTRO:
        if (M.t > 30 && !voice_busy()) {
            M.state = MM_PLAY;
            game_question_ready(); /* the board: timed as a whole */
            BOT("memory_ready cursor=%d\n", M.cur);
        }
        break;
    case MM_PLAY:
        M.quiet = voice_busy() ? 0 : M.quiet + 1;
        if (M.quiet == 15 * FPS)
            say("memory_intro");
        if (btn_pressed(BTN_LEFT))
            move_cursor(-1, 0);
        else if (btn_pressed(BTN_RIGHT))
            move_cursor(1, 0);
        else if (btn_pressed(BTN_UP))
            move_cursor(0, -1);
        else if (btn_pressed(BTN_DOWN))
            move_cursor(0, 1);
        else if (btn_pressed(BTN_A)) {
            M.quiet = 0;
            pick();
        } else if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) { /* "what do I do?" */
            M.quiet = 0;
            game_question_replay();
            say("memory_intro");
        }
        break;
    case MM_MISS: /* two different cards: look at them, then they turn back */
        if (M.t == SHOW_MISS) {
            turn(M.first, false);
            turn(M.second, false);
            sfx("whoosh");
            M.first = M.second = -1;
        }
        if (M.t > SHOW_MISS + FLIP) {
            if (!M.peeked && M.misses >= 2 * M.pairs) {
                M.peeked = true;
                say("memory_sbircia");
                for (int i = 0; i < M.n; i++)
                    if (!M.found[i])
                        turn(i, true);
                M.state = MM_PEEK;
                M.t = 0;
                BOT("hint\n");
            } else {
                M.state = MM_PLAY;
            }
        }
        break;
    case MM_PEEK:
        if (M.t == PEEK)
            for (int i = 0; i < M.n; i++)
                if (!M.found[i])
                    turn(i, false);
        if (M.t > PEEK + FLIP) {
            M.state = MM_PLAY; /* (the peek is a help in the middle of the board: its time goes on) */
            BOT("memory_ready cursor=%d\n", M.cur);
        }
        break;
    case MM_DONE:
        if (M.t > 150 && !voice_busy())
            game_round_done(GAME_MEMORY);
        break;
    }
}

static void draw_card(int i)
{
    int x = card_x(i), y = card_y(i);
    bool face = M.up[i];
    int w = CARD;
    if (M.flip_t[i] > 0) { /* turning: the card narrows, then shows its other side */
        int t = M.flip_t[i];
        if (t > FLIP / 2)
            face = !face;
        w = CARD * absi(2 * t - FLIP) / FLIP;
    }
    int dy = (M.state == MM_DONE && M.t < 90 && ((M.t / 4 + i) % 8) < 2) ? -3 : 0;
    gfx_set_clip(x + (CARD - w) / 2, 0, x + (CARD + w) / 2, SCREEN_H);
    if (face) {
        gfx_blit(gfx_sprite("memo_front"), x, y + dy, 0);
        char name[40];
        snprintf(name, sizeof(name), "rw_%s", ITEMS[M.item[i]].id);
        const sprite_t *ic = gfx_sprite(name);
        gfx_blit(ic, x + (CARD - ic->w) / 2, y + (CARD - ic->h) / 2 + dy, 0);
    } else {
        gfx_blit(gfx_sprite("memo_back"), x, y + dy, 0);
    }
    gfx_reset_state();
}

static void draw(void)
{
    amb_bg("bg_camerino");
    game_draw_stars(AREA_CX, M.found_pairs, M.pairs);
    for (int i = 0; i < M.n; i++)
        draw_card(i);
    if (M.state == MM_PLAY || M.state == MM_MISS) {
        const sprite_t *c = gfx_sprite("memo_cursor");
        int bob = swing((int)G.frame, 48, 1);
        gfx_blit(c, card_x(M.cur) - 3, card_y(M.cur) - 3 - bob, 0);
        ui_glow(card_x(M.cur) - 4, card_y(M.cur) - 4 - bob, CARD + 7, CARD + 7);
    }
    /* Deva in front of the mirror, and her reflection */
    hero_t refl = G.hero;
    refl.x = (GLASS_X0 + GLASS_X1) / 2;
    refl.y = GLASS_Y1 - 6;
    refl.mirror = true;
    gfx_set_clip(GLASS_X0, GLASS_Y0, GLASS_X1, GLASS_Y1);
    gfx_set_tint(rgb565(0xd9, 0xec, 0xff), 70);
    hero_draw(&refl, &G.prog);
    gfx_reset_state();
    hero_draw(&G.hero, &G.prog);
    fx_draw();
}

const scene_t SCENE_MEMORY = {enter, update, draw};
