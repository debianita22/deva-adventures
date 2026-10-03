/* Deva's Awesome Adventures - "prova dei tasti": in the first sessions, before
 * the menu, a big drawing of the console asks her to press the black cross
 * (Deva dances the step) and then the red button. Wrong buttons get a gentle
 * hint that says where to look ("a sinistra", "a destra").
 * SPDX-License-Identifier: MIT
 */
#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "hud.h"

#define ZOOM 3
#define CON_X (160 - HUD_CONSOLE_W * ZOOM / 2)
#define CON_Y 18
#define SESSIONS_WITH_TUTORIAL 3

enum { PV_CROCE, PV_ROSSO, PV_DONE };

static struct {
    int state, t, quiet, nags;
} V;

/* in the first sessions, once per session */
bool prova_wanted(void) { return !G.prova_done && G.prog.sessions <= SESSIONS_WITH_TUTORIAL; }

static void step(int state, const char *voice)
{
    V.state = state;
    V.t = V.quiet = 0;
    say_then(voice);
    BOT("prova step=%s\n", state == PV_CROCE ? "croce" : "rosso");
}

static void enter(void)
{
    hero_init(&G.hero, 160, 214);
    hero_play(&G.hero, HA_WAVE, 60);
    music_play(snd_find(SND_MUSIC, "palco"));
    voice_stop();
    step(PV_CROCE, "prova_croce");
}

static void praise(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_5"};
    sfx("ding");
    say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
}

static bool other_button(void)
{
    return btn_pressed(BTN_A) || btn_pressed(BTN_B) || btn_pressed(BTN_X) || btn_pressed(BTN_Y) ||
           btn_pressed(BTN_START) || btn_pressed(BTN_SELECT);
}

static int pressed_dir(void)
{
    static const int BTN[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
    for (int d = 0; d < 4; d++)
        if (btn_pressed(BTN[d]))
            return d;
    return -1;
}

static void dance_dir(int d)
{
    static const char *TONES[4] = {"tone_su", "tone_giu", "tone_sx", "tone_dx"};
    static const hero_pose_t POSE[4] = {POSE_UP, POSE_GIU, POSE_STEP, POSE_STEP};
    hero_move(&G.hero, POSE[d], d == 3, 26);
    sfx(TONES[d]);
}

static void update(void)
{
    V.t++;
    V.quiet = voice_busy() ? 0 : V.quiet + 1;
    int d = pressed_dir();
    switch (V.state) {
    case PV_CROCE:
        if (d >= 0) {
            dance_dir(d);
            fx_sparkles(CON_X + 9 * ZOOM, CON_Y + 15 * ZOOM, 14, 8);
            praise();
            step(PV_ROSSO, "prova_rosso");
            input_block(20);
        } else if (other_button()) {
            sfx("boop");
            say("prova_no_croce");
        } else if (idle_reminder(V.quiet, &V.nags, 8 * FPS)) {
            say("prova_croce");
            V.quiet = 0;
        }
        break;
    case PV_ROSSO:
        if (btn_pressed(BTN_A)) {
            sfx("star");
            fx_confetti(CON_X + 75 * ZOOM, CON_Y + 15 * ZOOM, 30);
            hero_play(&G.hero, HA_DANCE, 100);
            if (G.cfg.rumble)
                frontend_rumble(1, 10);
            praise();
            V.state = PV_DONE;
            V.t = 0;
            G.prova_done = true;
            BOT("prova step=fatto\n");
        } else if (d >= 0) {
            dance_dir(d); /* dancing is never wrong */
        } else if (other_button()) {
            sfx("boop");
            say("prova_no_rosso");
        } else if (idle_reminder(V.quiet, &V.nags, 8 * FPS)) {
            say("prova_rosso");
            V.quiet = 0;
        }
        break;
    case PV_DONE:
        if (V.t > 60 && !voice_busy())
            game_home();
        break;
    }
}

static void draw(void)
{
    amb_bg("bg_palco");
    int hl = V.state == PV_CROCE ? HUD_DPAD : HUD_A;
    bool lit = V.state == PV_DONE || ((V.t / 12) & 1) == 0;
    hud_console_zoom(CON_X, CON_Y, hl, lit, ZOOM);
    hero_draw(&G.hero, &G.prog);
    fx_draw();
}

const scene_t SCENE_PROVA = {enter, update, draw};
