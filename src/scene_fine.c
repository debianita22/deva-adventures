/* Deva's Awesome Adventures - end of the session: Deva says goodnight, falls asleep and
 * the curtain closes; then the core asks the frontend to quit.
 * SPDX-License-Identifier: MIT
 */
#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"

enum { FI_TALK, FI_SLEEP, FI_CURTAIN, FI_CLOSED };

static struct {
    int state, t, curtain;
} F;

static void enter(void)
{
    game_save();
    hero_init(&G.hero, 160, 212);
    hero_play(&G.hero, HA_WAVE, 0);
    music_play(snd_find(SND_MUSIC, "nanna"));
    say(name_voice("fine_nome") ? "fine_nome" : "fine");
    F.state = FI_TALK;
    F.t = F.curtain = 0;
}

static void update(void)
{
    F.t++;
    switch (F.state) {
    case FI_TALK:
        if (F.t > 30 && !voice_busy()) {
            hero_play(&G.hero, HA_SLEEP, 0);
            sfx("yawn");
            F.state = FI_SLEEP;
            F.t = 0;
        }
        break;
    case FI_SLEEP:
        if (F.t % 50 == 10)
            fx_zed(G.hero.x + 14, G.hero.y - 60);
        if (F.t == 70) /* a shooting star crosses the night sky */
            fx_trail(30, 22, 210, 70, 14);
        if (F.t > 200) {
            sfx("whoosh");
            F.state = FI_CURTAIN;
            F.t = 0;
        }
        break;
    case FI_CURTAIN:
        F.curtain = F.t * 176 / 90;
        if (F.curtain >= 176) {
            F.curtain = 176;
            F.state = FI_CLOSED;
            F.t = 0;
        }
        break;
    case FI_CLOSED:
        if (F.t == 8) /* goodnight: a few hearts in front of the curtain */
            fx_hearts(160, 150, 3);
        if (F.t == 90) {
            music_stop();
            G.quitting = true;
            frontend_shutdown();
        }
        break;
    }
}

static void draw(void)
{
    amb_bg("bg_notte");
    hero_draw(&G.hero, &G.prog);
    if (F.state != FI_CLOSED)
        fx_draw();
    if (F.curtain > 0) {
        const image_t *c = gfx_image("sipario");
        if (c) {
            gfx_blit_image(c, -c->w + F.curtain, 0, 0);
            gfx_blit_image(c, SCREEN_W - F.curtain, 0, GFX_FLIP_H);
        }
    }
    if (F.state == FI_CLOSED) /* the hearts, in front of the closed curtain */
        fx_draw();
}

const scene_t SCENE_FINE = {enter, update, draw};
