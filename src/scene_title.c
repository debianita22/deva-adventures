/* Deva's Awesome Adventures - title and main menu: the show opens, Deva says hello,
 * and a carousel of big cards waits under the logo: Gioca (highlighted: the red
 * button starts the game, as always), Camerino, Album, Salvataggi, Opzioni, Esci.
 * Salvataggi, Opzioni and Esci are for the grown-ups (0.14): L and R held down
 * together for two seconds (a ring of stars fills up); a tap of the red button
 * says "questo è per mamma e papà" and shows the gesture. Esci then asks "vuoi
 * andare a nanna?".
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
#include "hud.h"
#include "story.h"
#include "ui.h"

enum { MI_GIOCA, MI_CAMERINO, MI_ALBUM, MI_SALVATAGGI, MI_OPZIONI, MI_ESCI, MI_COUNT };

static const char *ICON[MI_COUNT] = {"mm_gioca", "mm_camerino", "mm_album", "mm_salvataggi", "mm_opzioni", "mm_esci"};
static const char *LABEL[MI_COUNT] = {"Gioca", "Camerino", "Album", "Salvataggi", "Opzioni", "Esci"};
static const char *VOICE[MI_COUNT] = {"m_gioca", "m_camerino", "m_album", "m_salvataggi", "m_opzioni", "m_esci"};
static const char *IDS[MI_COUNT] = {"gioca", "camerino", "album", "salvataggi", "opzioni", "esci"};

#define MC 60
#define STEP 72
#define MC_Y 84

static struct {
    int t, quiet, nags, sel, scroll;
    int hold;         /* frames L + R have been held on a grown-ups' card */
    int hint;         /* frames left showing "tieni premuti L e R" under the card */
    bool confirm;     /* "vuoi andare a nanna?" */
    int yes;          /* 1 = the moon (yes), 0 = play on */
    int go;           /* frames to the chosen item, 0 = none */
} T;

static int s_greeted_slot; /* hello once per profile */

static void select_item(int i)
{
    T.sel = clampi(i, 0, MI_COUNT - 1);
    G.title_sel = T.sel;
    BOT("title sel=%d item=%s\n", T.sel, IDS[T.sel]);
}

static void enter(void)
{
    memset(&T, 0, sizeof(T));
    hero_init(&G.hero, 160, 228);
    hero_play(&G.hero, HA_DANCE, 180);
    music_play(snd_find(SND_MUSIC, "palco"));
    select_item(G.title_sel);
    if (s_greeted_slot != G.slot) {
        s_greeted_slot = G.slot;
        bool going_on = story_active() && story_seen(SEEN_PROLOGO); /* a tale half told */
        bool all_told = G.cfg.story && story_arc_done(ARC_COUNT - 1); /* no new adventure to promise */
        const char *hi = going_on ? "saluto_continua" : all_told ? "saluto_gioca" : "saluto";
        char hi_name[32];
        snprintf(hi_name, sizeof(hi_name), "%s_nome", hi);
        say(name_voice(hi_name) ? hi_name : hi);
        say_then("premi_a");
    } else {
        say(VOICE[T.sel]);
    }
    T.scroll = T.sel * STEP * 16;
}

static bool grownup_item(int i) { return i == MI_SALVATAGGI || i == MI_OPZIONI || i == MI_ESCI; }

static void open_confirm(void)
{
    T.confirm = true;
    T.yes = 0;
    sfx("blip");
    say("m_esci");
    BOT("title confirm open\n");
}

static void choose(void)
{
    BOT("title_pick item=%s\n", IDS[T.sel]);
    switch (T.sel) {
    case MI_GIOCA: game_play(); break;
    case MI_CAMERINO: game_goto(SC_CAMERINO); break;
    case MI_ALBUM: game_goto(SC_ALBUM); break;
    case MI_SALVATAGGI: game_goto(SC_SALVATAGGI); break;
    case MI_OPZIONI: game_goto(SC_OPZIONI); break;
    default: game_goto(SC_FINE); break;
    }
}

static void update_confirm(void)
{
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir) {
        int yes = dir < 0 ? 1 : 0; /* the moon on the left, "play on" on the right */
        if (yes != T.yes) {
            T.yes = yes;
            sfx("blip");
            say(T.yes ? "m_nanna_si" : "m_nanna_no");
            BOT("title confirm yes=%d\n", T.yes);
        }
    } else if (btn_pressed(BTN_A)) {
        if (T.yes) {
            sfx("yawn");
            voice_stop();
            T.confirm = false;
            T.go = 1;
            BOT("title confirm done\n");
        } else {
            sfx("blip");
            T.confirm = false;
            say("m_nanna_no");
        }
    } else if (btn_pressed(BTN_B)) {
        T.confirm = false;
        sfx("blip");
    }
}

static void update(void)
{
    T.t++;
    int target = T.sel * STEP * 16;
    T.scroll += (target - T.scroll) / 5;
    if (absi(target - T.scroll) < 8)
        T.scroll = target;
    if (G.hero.anim == HA_IDLE && G.hero.t > 150) /* dance for 3 s, rest for 2.5 s */
        hero_play(&G.hero, HA_DANCE, 180);
    if (T.t % 40 == 0) /* disco ball glints */
        fx_sparkles(160, 26, 12, 2);
    if (T.t % 19 == 0) { /* a twinkle on a letter of the logo */
        const sprite_t *logo = gfx_sprite("logo");
        for (int k = 0; k < 8; k++) {
            int i = rng_range(0, logo->w - 1), j = rng_range(0, logo->h - 1);
            if (gfx_opaque(logo, i, j)) {
                fx_twinkle((SCREEN_W - logo->w) / 2 + i, 6 + j, 0);
                break;
            }
        }
    }
    if (T.go > 0) { /* a little dance, then the chosen place */
        if (++T.go > 24)
            choose();
        return;
    }
    if (T.confirm) {
        update_confirm();
        return;
    }
    if (T.hint > 0)
        T.hint--;
    /* the grown-ups' doors: L and R held down together for two seconds */
    if (grownup_item(T.sel) && grownup_held()) {
        if (++T.hold >= HOLD_FRAMES) {
            T.hold = 0;
            T.hint = 0;
            sfx("sparkle");
            BOT("title hold done\n");
            if (T.sel == MI_ESCI)
                open_confirm();
            else
                T.go = 1;
        } else if (T.hold % 10 == 0) {
            sfx("blip");
        }
        return;
    }
    if (T.hold > 0) {
        T.hold = 0;
        BOT("title hold released\n");
    }
    T.quiet = voice_busy() ? 0 : T.quiet + 1;
    if (idle_reminder(T.quiet, &T.nags, 12 * FPS)) {
        say(T.sel == MI_GIOCA ? "premi_a" : VOICE[T.sel]);
        T.quiet = 0;
    }
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir && T.sel + dir >= 0 && T.sel + dir < MI_COUNT) {
        select_item(T.sel + dir);
        sfx("blip");
        say(VOICE[T.sel]);
    } else if (btn_pressed(BTN_A) || btn_pressed(BTN_START)) {
        if (grownup_item(T.sel)) { /* not for her: say so, and show the grown-ups how */
            sfx("boop");
            say("m_grandi");
            T.hint = 4 * FPS;
            BOT("title grownup tap item=%s\n", IDS[T.sel]);
            return;
        }
        sfx("star");
        voice_stop();
        hero_play(&G.hero, HA_DANCE, 60);
        fx_sparkles(160, MC_Y + MC / 2, 26, 14);
        T.go = 1;
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
        say(T.sel == MI_GIOCA ? "premi_a" : VOICE[T.sel]);
    }
}

static void draw_card(int i, int cx)
{
    int x = cx - MC / 2, y = MC_Y, bob = 0;
    bool sel = i == T.sel;
    if (sel) {
        bob = T.go ? -swing(T.t, 12, 3) : -swing((int)G.frame, 48, 2);
        gfx_blit(gfx_sprite("mcard_sel"), x - 2, y - 2 + bob, 0);
    } else {
        gfx_blit(gfx_sprite("mcard"), x, y, 0);
    }
    const sprite_t *ic = gfx_sprite(ICON[i]);
    gfx_blit(ic, x + (MC - ic->w) / 2, y + (MC - ic->h) / 2 + bob, 0);
    if (grownup_item(i)) /* for the grown-ups */
        gfx_blit(gfx_sprite("mark_lucchetto"), x + MC - 13, y + 3 + bob, 0);
    if (sel) { /* the name, on a ribbon (for the grown-ups: she knows the pictures) */
        int w = gfx_text_width(LABEL[i]) + 12;
        gfx_round_rect(cx - w / 2, y + MC - 4 + bob, w, 13, COL_HOT, COL_INK);
        gfx_text(LABEL[i], cx, y + MC - 2 + bob, 0xffff, ALIGN_CENTER);
    }
    if (sel && T.hold > 0)
        ui_hold_stars(x - 4, y - 4 + bob, MC + 8, MC + 24, T.hold, HOLD_FRAMES);
    else if (sel)
        ui_glow(x - 3, y - 3 + bob, MC + 5, MC + 5);
    if (sel && grownup_item(i) && (T.hint > 0 || T.hold > 0)) { /* the gesture, written for the grown-ups */
        static const char *const TIP = "Per i grandi: tieni premuti L e R";
        int w = gfx_text_width(TIP) + 14;
        gfx_round_rect(cx - w / 2, y + MC + 13, w, 14, COL_CREAM, COL_INK);
        gfx_text(TIP, cx, y + MC + 16, COL_INK, ALIGN_CENTER);
    }
}

static void draw_confirm(void)
{
    gfx_fade(COL_DEEP, 140);
    ui_panel(40, 56, 240, 128);
    ui_title("A NANNA", 74);
    gfx_text("Vuoi uscire dal gioco?", 160, 88, COL_INK, ALIGN_CENTER);
    for (int k = 0; k < 2; k++) {
        int yes = 1 - k, cx = 116 + k * 88, y = 104;
        bool sel = yes == T.yes;
        int bob = sel ? -swing((int)G.frame, 48, 2) : 0;
        gfx_blit(gfx_sprite(sel ? "mcard_sel" : "mcard"), cx - (sel ? 32 : 30), y - (sel ? 2 : 0) + bob, 0);
        gfx_blit(gfx_sprite(yes ? "mm_esci" : "mm_gioca"), cx - 22, y + 8 + bob, 0);
    }
    gfx_text(T.yes ? "Sì, a nanna" : "No, gioco ancora", 160, 170, COL_HOT, ALIGN_CENTER);
}

static void draw(void)
{
    amb_bg("bg_palco");
    amb_petals(0, 0, SCREEN_W, SCREEN_H, 7, 11); /* pink petals falling on the stage */
    const sprite_t *logo = gfx_sprite("logo");
    int bob = ((T.t / 20) & 1) ? 1 : 0, lx = (SCREEN_W - logo->w) / 2;
    gfx_blit(logo, lx, 6 + bob, 0);
    int ph = T.t % 180; /* every three seconds a band of light runs across the logo */
    if (ph < 50)
        gfx_shine(logo, lx, 6 + bob, ph * (logo->w + 50) / 50 - 40, 9, 170);
    int off = T.scroll / 16;
    for (int i = 0; i < MI_COUNT; i++) {
        int cx = 160 + i * STEP - off;
        if (cx > -MC && cx < SCREEN_W + MC)
            draw_card(i, cx);
    }
    /* who is playing: her name in big letters (she knows it), with a crown */
    const char *name = child_name();
    int nw = gfx_text_big_width(name, 1);
    gfx_blit(gfx_sprite("mark_corona"), 8, 196, 0);
    if (nw <= 110)
        gfx_text_big(name, 8 + nw / 2, 220, NUM_NORMAL, 1);
    else
        gfx_text_shadow(name, 8, 214, COL_CREAM, COL_DEEP, ALIGN_LEFT);
    hero_draw(&G.hero, &G.prog);
    /* "Premi il bottone rosso": the console with its red A button glowing */
    if (T.sel == MI_GIOCA && !T.go && !T.confirm)
        hud_console(232, 184, HUD_A, (T.t / 12) % 3 != 2);
    fx_draw();
    if (T.confirm)
        draw_confirm();
}

const scene_t SCENE_TITLE = {enter, update, draw};
