/* Deva's Awesome Adventures - the dressing room, free play: every make-up item
 * she has won is on the table, the others are dark shapes still to be won.
 * The cross moves over the table, A puts an item on (or takes it off), the
 * mirror shows her; the yellow button goes back.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>

#include "anim.h"
#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "ui.h"

#define COLS 4
#define CELL 44
#define GX 138
#define GY 40
#define CARD 42

/* mirror glass in bg_camerino (see tools/art/backgrounds.py) */
#define GLASS_X0 19
#define GLASS_Y0 33
#define GLASS_X1 102
#define GLASS_Y1 171

static struct {
    int sel, t;
    bool changed;
    int lost; /* frames without putting anything on (0.14: then "to go back, the yellow button") */
} C;

static bool owned(int i) { return (G.prog.owned >> i) & 1u; }
static bool worn(int i) { return G.prog.worn[ITEMS[i].slot] == i; }

static void say_item(int i)
{
    char id[40];
    snprintf(id, sizeof(id), "t_%s", ITEMS[i].id);
    say(id);
}

static void select_item(int i)
{
    C.sel = clampi(i, 0, ITEM_COUNT - 1);
    BOT("camerino sel=%d item=%s owned=%d worn=%d\n", C.sel, ITEMS[C.sel].id, owned(C.sel), worn(C.sel));
}

static void enter(void)
{
    C.t = 0;
    C.changed = false;
    hero_init(&G.hero, 110, 212);
    hero_play(&G.hero, HA_WAVE, 70);
    music_play(snd_find(SND_MUSIC, "palco"));
    select_item(0);
    say("cam_intro");
}

static void leave(void)
{
    if (C.changed)
        game_save();
    sfx("blip");
    G.title_sel = 0; /* back on "Gioca": the red button plays (0.14: else it came back here, round and round) */
    game_goto(SC_TITLE);
}

static void update(void)
{
    C.t++;
    if (G.hero.anim == HA_IDLE && G.hero.t > 240)
        hero_play(&G.hero, HA_DANCE, 90);
    /* lost in the dressing room? how to go back: after 25 s without trying anything on, then every 30 s */
    if (!voice_busy() && ++C.lost == 25 * FPS) {
        say("torna_giallo");
        C.lost = -5 * FPS;
    }
    int dx = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    int dy = btn_pressed(BTN_UP) ? -1 : (btn_pressed(BTN_DOWN) ? 1 : 0);
    if (dx || dy) {
        int col = C.sel % COLS + dx, row = C.sel / COLS + dy;
        int to = row * COLS + col;
        if (col >= 0 && col < COLS && row >= 0 && to < ITEM_COUNT) {
            select_item(to);
            sfx("blip");
            if (owned(C.sel))
                say_item(C.sel);
            else
                voice_stop();
        }
    } else if (btn_pressed(BTN_A)) {
        int i = C.sel;
        if (!owned(i)) {
            sfx("boop");
            say("cam_bloccato");
        } else if (worn(i)) {
            C.lost = 0;
            G.prog.worn[ITEMS[i].slot] = -1;
            sfx("whoosh");
            hero_play(&G.hero, HA_OH, 40);
            C.changed = true;
            BOT("camerino wear item=%s on=0\n", ITEMS[i].id);
        } else {
            C.lost = 0;
            progress_wear(&G.prog, i);
            sfx("sparkle");
            hero_play(&G.hero, HA_DANCE, 110);
            fx_sparkles(G.hero.x, G.hero.y - 44, 20, 16);
            fx_hearts(G.hero.x + 4, G.hero.y - 70, 2);
            say_item(i);
            C.changed = true;
            BOT("camerino wear item=%s on=1\n", ITEMS[i].id);
        }
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_START)) {
        leave();
    } else if (btn_pressed(BTN_Y)) {
        say("cam_intro");
    }
}

static void draw(void)
{
    amb_bg("bg_camerino");
    char name[40];
    for (int i = 0; i < ITEM_COUNT; i++) {
        int x = GX + (i % COLS) * CELL, y = GY + (i / COLS) * CELL;
        bool sel = i == C.sel;
        int bob = sel ? -swing((int)G.frame, 48, 1) : 0;
        ui_card(x, y + bob, CARD, CARD, sel);
        snprintf(name, sizeof(name), "rw_%s", ITEMS[i].id);
        const sprite_t *ic = gfx_sprite(name);
        int ix = x + (CARD - ic->w) / 2, iy = y + (CARD - ic->h) / 2 + bob;
        if (owned(i))
            gfx_blit(ic, ix, iy, 0);
        else
            ui_silhouette(ic, ix, iy);
        if (worn(i))
            gfx_blit(gfx_sprite("mark_ok"), x + CARD - 13, y - 4 + bob, 0);
    }
    /* her reflection, as in the reward scene */
    hero_t refl = G.hero;
    refl.x = (GLASS_X0 + GLASS_X1) / 2;
    refl.y = GLASS_Y1 - 6;
    refl.mirror = true;
    gfx_set_clip(GLASS_X0, GLASS_Y0, GLASS_X1, GLASS_Y1);
    gfx_set_tint(rgb565(0xd9, 0xec, 0xff), 70);
    hero_draw(&refl, &G.prog);
    gfx_reset_state();
    hero_draw(&G.hero, &G.prog);
    gfx_text_big("CAMERINO", GX + COLS * CELL / 2, 22, NUM_SELECTED, 1);
    if (C.t > 60)
        ui_back_hint();
    fx_draw();
}

const scene_t SCENE_CAMERINO = {enter, update, draw};
