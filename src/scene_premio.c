/* Deva's Awesome Adventures - dressing room: after a full row of stars she picks one make-up
 * item out of three; Deva wears it from then on. Once every make-up item is won,
 * the prize is a sticker for the album (one of three). Then back to the menu (or
 * to bed).
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

#define RCARD 52
#define RCARD_Y 110
#define ICON 36

static const int RCARD_X[3] = {140, 196, 252};

/* mirror glass in bg_camerino (see tools/art/backgrounds.py) */
#define GLASS_X0 19
#define GLASS_Y0 33
#define GLASS_X1 102
#define GLASS_Y1 171

enum { PR_CHOOSE, PR_DONE };

#define FLY 26 /* frames of the flight of the chosen item to Deva */

static struct {
    int items[3], n, sel, state, t, chosen;
    int fly, fly_from;   /* the item flying to her (0 = none), from that card */
    bool all_owned;
    bool stickers;   /* the items are stickers (STICKERS[]), not make-up */
    const char *ask; /* "scegli il tuo trucco" / "scegli un adesivo" */
    int quiet, nags; /* idle reminders (0.14): the choice, then the red button to go on */
} P;

static progress_t s_before; /* her look before the new item: shown while it flies to her */

static void say_item(int item)
{
    char id[40];
    snprintf(id, sizeof(id), P.stickers ? "w_%s" : "t_%s", P.stickers ? STICKERS[item] : ITEMS[item].id);
    say(id);
}

static void shuffle(int *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rng_range(0, i), tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

/* every make-up item won: three stickers she has not got yet (false: all of them too) */
static bool offer_stickers(void)
{
    int pool[STICKER_COUNT], np = 0;
    for (int i = 0; i < STICKER_COUNT; i++)
        if (!((G.prog.stickers >> i) & 1u))
            pool[np++] = i;
    if (np == 0)
        return false;
    shuffle(pool, np);
    P.stickers = true;
    P.n = np < 3 ? np : 3;
    for (int i = 0; i < P.n; i++)
        P.items[i] = pool[i];
    BOT("offer stickers n=%d items=%s,%s,%s\n", P.n, STICKERS[P.items[0]], P.n > 1 ? STICKERS[P.items[1]] : "-",
        P.n > 2 ? STICKERS[P.items[2]] : "-");
    return true;
}

static bool worn(int item) { return G.prog.worn[ITEMS[item].slot] == item; }

static void offer(void)
{
    int pool[32], np = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        if (!(G.prog.owned & (1u << i)))
            pool[np++] = i;
    P.all_owned = np == 0;
    P.stickers = false;
    if (P.all_owned && offer_stickers())
        return;
    /* shuffle the new items, take three; top up with owned ones not worn */
    shuffle(pool, np);
    P.n = 0;
    for (int i = 0; i < np && P.n < 3; i++)
        P.items[P.n++] = pool[i];
    int extra[32], ne = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        if ((G.prog.owned & (1u << i)) && !worn(i))
            extra[ne++] = i;
    shuffle(extra, ne);
    for (int i = 0; i < ne && P.n < 3; i++)
        P.items[P.n++] = extra[i];
    BOT("offer n=%d items=%s,%s,%s\n", P.n, P.n > 0 ? ITEMS[P.items[0]].id : "-",
        P.n > 1 ? ITEMS[P.items[1]].id : "-", P.n > 2 ? ITEMS[P.items[2]].id : "-");
}

static void enter(void)
{
    hero_init(&G.hero, 110, 212);
    music_play(snd_find(SND_MUSIC, "palco"));
    offer();
    P.sel = P.n == 3 ? 1 : 0;
    P.state = PR_CHOOSE;
    P.t = 0;
    P.chosen = -1;
    P.fly = 0;
    if (P.stickers) /* the first sticker: "hai vinto tutti i trucchi! adesso gli adesivi..." */
        P.ask = G.prog.stickers == 0 ? "premio_adesivo_primo" : "premio_adesivo";
    else
        P.ask = P.all_owned ? "premio_tutti" : "premio_scegli";
    if (G.from_round) {
        sfx("fanfare");
        hero_play(&G.hero, HA_DANCE, 90);
        say(voice_variant("round_fine"));
        say_then(P.ask);
    } else {
        say(P.ask);
    }
    if (P.n == 0) /* nothing to offer: go straight on */
        P.state = PR_DONE;
}

static void next_scene(void)
{
    G.from_round = false;
    game_goto(session_over() ? SC_FINE : SC_MENU);
}

/* where the flying item is: from the card up in an arc to her face, slowing down */
static void fly_pos(int t, int *x, int *y)
{
    int x0 = RCARD_X[P.fly_from] + RCARD / 2, y0 = RCARD_Y + RCARD / 2;
    int x1 = G.hero.x, y1 = G.hero.y - 48;
    int u = clampi(t * 256 / FLY, 0, 256), e = 256 - (256 - u) * (256 - u) / 256;
    *x = x0 + (x1 - x0) * e / 256;
    *y = y0 + (y1 - y0) * e / 256 - e * (256 - e) / 256 * 40 / 64;
}

static void update(void)
{
    P.t++;
    if (P.fly > 0) {
        int x, y;
        fly_pos(P.fly, &x, &y);
        if (++P.fly > FLY) { /* she has it: sparkles and hearts */
            P.fly = 0;
            sfx("pop");
            fx_sparkles(G.hero.x, G.hero.y - 44, 20, 16);
            fx_hearts(G.hero.x + 4, G.hero.y - 70, 4);
        } else if (P.fly % 2 == 0) {
            int dx, dy;
            rng_pair(-3, 3, -3, 3, &dx, &dy);
            fx_twinkle(x + dx, y + dy, 0);
        }
    }
    P.quiet = voice_busy() ? 0 : P.quiet + 1;
    if (idle_reminder(P.quiet, &P.nags, 12 * FPS)) { /* still there? what to do now, without nagging */
        say(P.state == PR_CHOOSE ? P.ask : voice_variant("continua"));
        P.quiet = 0;
    }
    if (P.state == PR_CHOOSE) {
        int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
        if (dir && P.sel + dir >= 0 && P.sel + dir < P.n) {
            P.sel += dir;
            sfx("blip");
            say_item(P.items[P.sel]);
        } else if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            say(P.ask);
        } else if (btn_pressed(BTN_A)) {
            P.chosen = P.items[P.sel];
            if (P.stickers) { /* into the album */
                G.prog.stickers |= (uint64_t)1 << P.chosen;
                BOT("reward sticker=%s\n", STICKERS[P.chosen]);
                fx_sparkles(RCARD_X[P.sel] + RCARD / 2, RCARD_Y + RCARD / 2, 24, 18);
                fx_burst(RCARD_X[P.sel] + RCARD / 2, RCARD_Y + RCARD / 2, 14);
            } else { /* it flies to her, and she wears it when it gets there */
                s_before = G.prog;
                progress_wear(&G.prog, P.chosen);
                BOT("reward item=%s\n", ITEMS[P.chosen].id);
                P.fly = 1;
                P.fly_from = P.sel;
            }
            game_save();
            sfx("sparkle");
            hero_play(&G.hero, HA_DANCE, 170);
            say_item(P.chosen);
            say_then(P.stickers ? "premio_adesivo_bello" : "premio_bella");
            say_then(voice_variant("continua"));
            P.state = PR_DONE;
            P.t = 0;
            input_block(45);
        }
    } else if (btn_pressed(BTN_A) || btn_pressed(BTN_START)) {
        next_scene();
    }
}

static void draw(void)
{
    amb_bg("bg_camerino");
    char name[40];
    for (int i = 0; i < P.n; i++) {
        int x = RCARD_X[i], y = RCARD_Y;
        bool sel = P.state == PR_CHOOSE ? i == P.sel : P.items[i] == P.chosen;
        int bob = 0;
        if (sel) {
            bob = -swing((int)G.frame, 48, 1);
            gfx_blit(gfx_sprite("rcard_sel"), x - 2, y - 2 + bob, 0);
            if (P.state == PR_CHOOSE)
                ui_glow(x - 3, y - 3 + bob, RCARD + 5, RCARD + 5);
        } else {
            gfx_blit(gfx_sprite(P.state == PR_DONE ? "rcard_off" : "rcard"), x, y, 0);
        }
        if (P.stickers) {
            snprintf(name, sizeof(name), "pic_%s", STICKERS[P.items[i]]);
            const sprite_t *s = gfx_sprite(name);
            ui_sticker(s, x + (RCARD - s->w) / 2, y + (RCARD - s->h) / 2 + bob);
            continue;
        }
        snprintf(name, sizeof(name), "rw_%s", ITEMS[P.items[i]].id);
        gfx_blit(gfx_sprite(name), x + (RCARD - ICON) / 2, y + (RCARD - ICON) / 2 + bob, 0);
    }
    /* her reflection: same pose, mirrored, a little misty, inside the glass */
    const progress_t *look = P.fly > 0 ? &s_before : &G.prog;
    hero_t refl = G.hero;
    refl.x = (GLASS_X0 + GLASS_X1) / 2;
    refl.y = GLASS_Y1 - 6;
    refl.mirror = true;
    gfx_set_clip(GLASS_X0, GLASS_Y0, GLASS_X1, GLASS_Y1);
    gfx_set_tint(rgb565(0xd9, 0xec, 0xff), 70);
    hero_draw(&refl, look);
    gfx_reset_state();
    hero_draw(&G.hero, look);
    if (P.fly > 0) { /* the new item on its way */
        int fx_, fy;
        fly_pos(P.fly, &fx_, &fy);
        snprintf(name, sizeof(name), "rw_%s", ITEMS[P.chosen].id);
        const sprite_t *ic = gfx_sprite(name);
        gfx_blit(ic, fx_ - ic->w / 2, fy - ic->h / 2, 0);
    }
    if (P.state == PR_DONE && P.t > 90 && !voice_busy() && ((P.t / 24) & 1))
        gfx_blit(gfx_sprite("btn_a"), 206, 176, 0);
    fx_draw();
}

const scene_t SCENE_PREMIO = {enter, update, draw};
