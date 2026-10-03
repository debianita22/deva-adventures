/* Deva's Awesome Adventures - the menu: a carousel of game cards on the stage.
 * Left/right scroll, the highlighted card says its name, five little stars
 * show its level. Games still locked are "?" cards at the end ("arriva
 * presto"); a round that unlocks one brings her here to see the "?" turn
 * into the new game. The highlight starts on the game played least in this
 * session, so a child who just presses A rotates through all of them.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>

#include "anim.h"
#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "story.h"
#include "ui.h"

#define MC 60      /* card size */
#define STEP 72    /* distance between card centres */
#define MC_Y 70
#define REVEAL_AT 70 /* frames before a newly unlocked card turns around */
#define FLIP 9       /* frames of each half of the turn */
#define FB_W (MC + 8)  /* the card drawn aside while it turns */
#define FB_H (MC + 26)
#define KEY 0xF81F

static uint16_t s_flip[FB_W * FB_H];

typedef struct {
    int game;
    bool locked;
    bool duel;           /* the tale: "sfida il mostro" (game = -1) */
} entry_t;

static struct {
    entry_t e[GAME_COUNT + 1];
    int n, sel, t, quiet, nags;
    int scroll;          /* x offset of the carousel, in 1/16 px */
    bool chosen;
    int reveal;          /* entry being unlocked, -1 = none */
    bool duel_after;     /* the wand is full too: after the new game, "sfida il mostro" */
    int charge_t;        /* a star just added to the wand, frames */
    int sel_t;           /* frames since the highlighted card changed (0.13: its icon pops up) */
} M;

static const char *entry_id(int i) { return M.e[i].duel ? "sfida" : GAME_IDS[M.e[i].game]; }

static bool unlocked(int g) { return game_is_unlocked(&G.prog, &G.cfg, g); }

static void build_entries(void)
{
    M.n = 0;
    if (story_charged()) /* the charged wand: the duel comes first */
        M.e[M.n++] = (entry_t){-1, false, true};
    for (int g = 0; g < GAME_COUNT; g++) /* open games first, in menu order */
        if (unlocked(g))
            M.e[M.n++] = (entry_t){g, false, false};
    /* 0.14: one "?" card only, the game that opens next (fourteen "?" in a row were a long walk of
       closed doors for a small child) */
    int next = progress_next_locked(&G.prog);
    if (next >= 0 && !unlocked(next))
        M.e[M.n++] = (entry_t){next, true, false};
}

static void say_entry(int i, bool queue)
{
    char id[24];
    if (M.e[i].locked)
        snprintf(id, sizeof(id), "bloccato");
    else if (M.e[i].duel)
        snprintf(id, sizeof(id), "%s", story_line("g_sfida"));
    else
        snprintf(id, sizeof(id), "g_%s", GAME_IDS[M.e[i].game]);
    if (queue)
        say_then(id);
    else
        say(id);
}

/* least played this session among the open games; ties go to the one after the last played */
static int suggested(void)
{
    if (M.n > 0 && M.e[0].duel)
        return 0;
    int best = -1;
    for (int k = 1; k <= GAME_COUNT; k++) {
        int g = (G.last_game + k) % GAME_COUNT;
        if (!unlocked(g))
            continue;
        if (best < 0 || G.rounds_session[g] < G.rounds_session[best])
            best = g;
    }
    for (int i = 0; i < M.n; i++)
        if (M.e[i].game == best)
            return i;
    return 0;
}

static void select_entry(int i)
{
    M.sel = clampi(i, 0, M.n - 1);
    M.sel_t = 0;
    BOT("menu sel=%d game=%s locked=%d\n", M.sel, entry_id(M.sel), M.e[M.sel].locked);
}

static void enter(void)
{
    build_entries();
    M.t = M.quiet = 0;
    M.chosen = false;
    M.reveal = -1;
    hero_init(&G.hero, 160, 214);
    hero_play(&G.hero, HA_WAVE, 70);
    music_play(snd_find(SND_MUSIC, "palco"));
    M.charge_t = 0;
    M.duel_after = false;
    if (G.prog.reveal >= 0) { /* the new game shows as "?" for a moment, then turns around */
        for (int i = 0; i < M.n; i++)
            if (M.e[i].game == G.prog.reveal && !M.e[i].locked)
                M.reveal = i;
        G.prog.reveal = -1;
        game_save();
    }
    bool charged_now = false;
    if (G.just_charged && story_active()) { /* a star flies into the wand */
        charged_now = true;
        M.charge_t = 1;
        sfx("sparkle");
        if (story_charged() && M.reveal >= 0)
            M.duel_after = true; /* the new game first, then the duel (and its card) */
        else
            say(story_charged() ? story_line("bacchetta_pronta") : "bacchetta_su");
    }
    G.just_charged = false;
    BOT("menu sfida=%d\n", M.n > 0 && M.e[0].duel);
    if (M.reveal >= 0) {
        M.e[M.reveal].locked = true;
        select_entry(M.reveal);
        sfx("fanfare");
        if (charged_now && !M.duel_after)
            say_then("sblocco");
        else
            say("sblocco");
    }
    if (M.reveal < 0) {
        select_entry(suggested());
        /* the time is nearly up: she hears it now, not when the curtain comes down (0.14) */
        const char *ask = voice_variant("menu_scegli");
        if (session_ending() && !G.last_told) {
            G.last_told = true;
            ask = "ultimo_gioco";
            BOT("menu last game\n");
        }
        if (!charged_now)
            say(ask);
        else if (!(M.n > 0 && M.e[0].duel))
            say_then(ask);
        if (!(charged_now && M.e[M.sel].duel)) /* "la bacchetta è carica... sfida il mostro" says it */
            say_entry(M.sel, true);
    }
    M.scroll = M.sel * STEP * 16;
}

static void update(void)
{
    M.t++;
    M.sel_t++;
    /* the carousel glides to the highlighted card */
    int target = M.sel * STEP * 16;
    M.scroll += (target - M.scroll) / 5;
    if (absi(target - M.scroll) < 8)
        M.scroll = target;

    if (M.reveal >= 0) {
        if (M.t == REVEAL_AT) {
            M.e[M.reveal].locked = false;
            sfx("sparkle");
            fx_sparkles(160, MC_Y + MC / 2, 30, 20);
            fx_burst(160, MC_Y + MC / 2, 14);
            fx_hearts(G.hero.x + 4, G.hero.y - 70, 3);
            hero_play(&G.hero, HA_DANCE, 110);
            say_entry(M.reveal, true);
            BOT("menu_unlocked game=%s\n", entry_id(M.reveal));
        }
        if (M.t > REVEAL_AT + FLIP && !voice_busy()) { /* the card has finished turning */
            M.reveal = -1;
            if (M.duel_after) { /* and now the monster: the wand is ready */
                M.duel_after = false;
                select_entry(0);
                sfx("sparkle");
                say(story_line("bacchetta_pronta"));
                M.quiet = 0;
            }
        }
        return;
    }
    if (M.charge_t > 0 && ++M.charge_t > 60)
        M.charge_t = 0;
    if (M.chosen) {
        /* a short dance while the voice confirms the name, then the game */
        if (M.t > 30 && (!voice_busy() || M.t > 120)) {
            if (M.e[M.sel].duel)
                mappa_open(MAP_GO); /* Deva walks to the monster */
            else
                game_goto(game_scene(M.e[M.sel].game));
        }
        return;
    }
    if (G.hero.anim == HA_IDLE && G.hero.t > 200)
        hero_play(&G.hero, HA_DANCE, 110);
    if (session_over() && !voice_busy()) { /* the time is up while she waits here: goodnight */
        game_goto(SC_FINE);
        return;
    }
    M.quiet = voice_busy() ? 0 : M.quiet + 1;
    if (idle_reminder(M.quiet, &M.nags, 12 * FPS)) {
        say(voice_variant("menu_scegli"));
        say_entry(M.sel, true);
        M.quiet = 0;
    }
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir && M.sel + dir >= 0 && M.sel + dir < M.n) {
        select_entry(M.sel + dir);
        sfx("blip");
        say_entry(M.sel, false);
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
        say(voice_variant("menu_scegli"));
    } else if (btn_pressed(BTN_A)) { /* (not START: held down, it opens the pause) */
        if (M.e[M.sel].locked) {
            sfx("boop");
            say_entry(M.sel, false);
            return;
        }
        sfx("star");
        say_entry(M.sel, false);
        hero_play(&G.hero, HA_DANCE, 60);
        fx_sparkles(160, MC_Y + MC / 2, 26, 14);
        fx_burst(160, MC_Y + MC / 2, 10);
        M.chosen = true;
        M.t = 0;
        BOT("menu_pick game=%s\n", entry_id(M.sel));
    }
}

static void draw_card(int i, int cx, int y)
{
    const entry_t *e = &M.e[i];
    int x = cx - MC / 2, bob = 0;
    bool sel = i == M.sel;
    if (sel) {
        bob = M.chosen ? -swing(M.t, 15, 3) : -swing((int)G.frame, 48, 2);
        gfx_blit(gfx_sprite("mcard_sel"), x - 2, y - 2 + bob, 0);
        ui_glow(x - 3, y - 3 + bob, MC + 5, MC + 5);
    } else {
        gfx_blit(gfx_sprite("mcard"), x, y, 0);
    }
    if (e->locked) {
        /* a mystery card: a big question mark that wiggles */
        const sprite_t *qm = gfx_sprite("qmark");
        int wig = (M.reveal == i && M.t < REVEAL_AT) ? ((M.t / 3) & 1) * 2 - 1 : 0;
        gfx_blit_scaled(qm, cx - qm->w + wig, y + (MC - qm->h * 2) / 2 + bob, 2, 0);
        return;
    }
    if (e->duel) { /* the monster of the next place, trembling */
        char name[32];
        snprintf(name, sizeof(name), "menu_sfida_%s", story_ch(story_chapter())->foe);
        const sprite_t *ic = gfx_sprite(name);
        int sh = ((G.frame / 4) & 1) && ((G.frame / 40) % 3 == 0) ? 1 : 0;
        gfx_blit(ic, x + (MC - ic->w) / 2 + sh, y + (MC - ic->h) / 2 + bob, 0);
        const sprite_t *w = gfx_sprite("bacchetta");
        gfx_blit(w, x + MC / 2 - w->w / 2, y + MC + 2, 0);
        return;
    }
    const sprite_t *ic = gfx_sprite(GAME_ICONS[e->game]);
    if (sel) { /* 0.13: the icon pops up (taller, thinner) when the arrows reach it, then breathes */
        int sdy = M.sel_t < 12 ? swing(M.sel_t, 12, 4) : swing((int)G.frame, 90, 1);
        gfx_blit_squash(ic, x + MC / 2, y + (MC - ic->h) / 2 + bob + ic->h, 1, 0, ic->h / 2, sdy, -sdy / 2);
    } else {
        gfx_blit(ic, x + (MC - ic->w) / 2, y + (MC - ic->h) / 2 + bob, 0);
    }
    /* level 1..5 as little stars under the card */
    int lvl = clampi(G.prog.level[e->game], 1, 5);
    for (int k = 0; k < 5; k++)
        gfx_blit(gfx_sprite(k < lvl ? "lstar_on" : "lstar_off"), x + 1 + k * 12, y + MC + 5, 0);
}

static void draw(void)
{
    amb_bg("bg_palco");
    int off = M.scroll / 16;
    for (int i = 0; i < M.n; i++) {
        int cx = 160 + i * STEP - off, d = M.t - REVEAL_AT;
        if (cx <= -MC || cx >= SCREEN_W + MC)
            continue;
        if (i == M.reveal && d >= -FLIP && d < FLIP) { /* the "?" turns round: the new game is on the back */
            gfx_target(s_flip, FB_W, FB_H);
            gfx_fill_rect(0, 0, FB_W, FB_H, KEY);
            draw_card(i, FB_W / 2, 4);
            gfx_target(NULL, 0, 0);
            int dw = FB_W * absi(d < 0 ? d : d + 1) / FLIP;
            gfx_blit_hsqueeze(s_flip, FB_W, FB_H, KEY, cx, MC_Y - 4, dw > 2 ? dw : 2);
        } else {
            draw_card(i, cx, MC_Y);
        }
    }
    /* more cards on a side: a small arrow says so */
    if (M.sel > 0 && ((G.frame / 20) & 1))
        gfx_blit(gfx_sprite("arrow_left"), 2, MC_Y + 18, 0);
    if (M.sel < M.n - 1 && ((G.frame / 20) & 1))
        gfx_blit(gfx_sprite("arrow_right"), SCREEN_W - 26, MC_Y + 18, 0);
    if (story_active()) { /* the wand of the tale and its charge */
        gfx_blit(gfx_sprite("bacchetta"), 6, 4, 0);
        int need = story_charge_needed();
        for (int i = 0; i < need; i++) {
            bool on = i < G.prog.charge;
            if (on && i == G.prog.charge - 1 && M.charge_t > 0 && M.charge_t < 30 && ((M.charge_t / 4) & 1))
                on = false; /* the new star blinks in */
            gfx_blit(gfx_sprite(on ? "lstar_on" : "lstar_off"), 18 + i * 12, 9, 0);
        }
        if (M.charge_t == 2)
            fx_sparkles(18 + (G.prog.charge - 1) * 12 + 5, 14, 10, 10);
    }
    hero_draw(&G.hero, &G.prog);
    if (!M.chosen && M.reveal < 0 && M.t > 150 && ((M.t / 24) % 3 != 2) && !M.e[M.sel].locked)
        gfx_blit(gfx_sprite("btn_a"), 160 + MC / 2 - 10, MC_Y - 10, 0);
    fx_draw();
}

const scene_t SCENE_MENU = {enter, update, draw};
