/* Deva's Awesome Adventures - the map of the Regno delle Stelle.
 *
 * Grey under the witch's spell (dark under the wizard's, in the second
 * adventure); every place Deva frees gets its colours (its stars) back in a
 * circle that grows around it. Deva stands where she arrived last; the
 * monster of the next place waits there with its gem (its star). The wand at
 * the top left shows its charge: with the red button she goes to play (the
 * menu) or, with a charged wand, walks to the monster and the duel begins.
 * SPDX-License-Identifier: MIT
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "story.h"

#define GROW 70 /* frames of the colours coming back */
#define WALK 60 /* frames of the walk to the monster */

enum { MP_FREE, MP_SAY, MP_READY, MP_WALK };

static struct {
    int state, t, event, quiet;
    int loc_x[6], loc_y[6], reg_x[6], reg_y[6], reg_r[6];
    int grow;         /* radius of the place being freed */
    bool foe_shown;   /* the next monster is on the map */
    char line[32];    /* what the voice says: who is next... (B/Y repeats it) */
    char line2[32];   /* ...and what to do now */
    bool reminded;    /* said again once after a long silence */
    bool recap;       /* "bentornata, contiamo le gemme...": the gems found hop as they are counted (0.12) */
    int hold_a;       /* frames A has been held while the guide speaks: a long press skips (the grown-ups) */
} M;

/* the numbers of the recap: the treasures are all feminine (gemme, stelle, note, chiavi) */
static const char *const COUNT_VOICE[4] = {"n01f", "n02", "n03", "n04"};

static void load_anchors(void)
{
    const arc_t *ar = story_arc_def();
    char a[24];
    int dummy;
    for (int i = 0; i < 6; i++) {
        M.loc_x[i] = ar->loc[i][0], M.loc_y[i] = ar->loc[i][1];
        M.reg_x[i] = ar->reg[i][0], M.reg_y[i] = ar->reg[i][1], M.reg_r[i] = ar->reg[i][2];
        snprintf(a, sizeof(a), "%sloc%d", ar->anchor, i);
        gfx_anchor(a, &M.loc_x[i], &M.loc_y[i]);
        snprintf(a, sizeof(a), "%sreg%d", ar->anchor, i);
        gfx_anchor(a, &M.reg_x[i], &M.reg_y[i]);
        snprintf(a, sizeof(a), "%srad%d", ar->anchor, i);
        gfx_anchor(a, &M.reg_r[i], &dummy);
    }
}

void mappa_open(int event)
{
    G.map_event = event;
    game_goto(SC_MAPPA);
}

static void speak(const char *id)
{
    snprintf(M.line, sizeof(M.line), "%s", id);
    M.line2[0] = 0;
    say(id);
}

static void repeat(void) /* B, or a long silence: all of it again */
{
    if (M.line[0])
        say(M.line);
    if (M.line2[0])
        say_then(M.line2);
}

/* what the guide says when the map is ready: who waits at the next place (the
   first time), then what to do - play to charge the wand, or go and duel */
static void say_status(void)
{
    int ch = story_chapter();
    if (ch >= CH_COUNT) {
        speak(story_arc_def()->map_done);
        return;
    }
    const char *todo = story_charged() ? story_line("mappa_sfida") : "mappa_gioca";
    char cap[24];
    snprintf(cap, sizeof(cap), "%s%d", story_arc_def()->cap, clampi(ch + 1, 1, 9));
    uint32_t bit = (uint32_t)SEEN_CAP1 << ch;
    if (!story_seen(bit) || !G.map_told) { /* new, or a new day: who waits there, and why */
        char rit[24], left[24];
        snprintf(rit, sizeof(rit), "rit_%d", story_arc() + 1);
        snprintf(left, sizeof(left), "mancano_%d", CH_COUNT - ch);
        if (!G.map_told && story_seen(bit) && ch >= 1 && snd_exists(SND_VOICE, rit)) {
            /* back another day: "Bentornata, Deva! Contiamo le gemme che abbiamo ritrovato! Una, due!
               Ne mancano tre!" - then who is next, as always */
            say(rit);
            for (int i = 0; i < ch && i < 4; i++)
                say_then(COUNT_VOICE[i]);
            say_then(left);
            say_then(cap);
            M.recap = true;
            BOT("mappa recap found=%d\n", ch);
        } else {
            speak(cap);
        }
        say_then(todo);
        story_mark_seen(bit);
        game_save();
    } else {
        speak(todo);
    }
    G.map_told = true;
    /* B tells the whole of it: who is next, then what to do */
    snprintf(M.line, sizeof(M.line), "%s", cap);
    snprintf(M.line2, sizeof(M.line2), "%s", todo);
}

static void enter(void)
{
    memset(&M, 0, sizeof(M));
    load_anchors();
    M.event = G.map_event;
    G.map_event = MAP_PLAIN;
    music_play(snd_find(SND_MUSIC, story_arc_def()->music));
    int ch = story_chapter();
    hero_init(&G.hero, M.loc_x[ch], M.loc_y[ch]);
    if (!story_seen(SEEN_PROLOGO) && story_active()) {
        racconto_play(R_PROLOGO, SC_MAPPA);
        return;
    }
    if (M.event == MAP_GO && story_charged()) { /* from the menu: straight to the monster */
        M.state = MP_WALK;
        M.foe_shown = true;
        M.grow = 999;
        BOT("mappa go=%s\n", story_ch(ch)->foe);
        return;
    }
    if (M.event == MAP_FREED && ch >= 1) {
        /* the place just freed gets its colours back */
        M.state = MP_FREE;
        sfx("trasforma");
        static char id[24];
        snprintf(id, sizeof(id), "%s%d", story_arc_def()->freed, clampi(ch, 1, 9));
        if (ch < CH_COUNT)
            speak(id);
        hero_play(&G.hero, HA_DANCE, 120);
    } else {
        M.state = MP_SAY;
        M.grow = 999;
        M.foe_shown = true;
        say_status();
    }
    BOT("mappa chapter=%d charge=%d/%d event=%d arc=%d\n", ch, G.prog.charge, story_charge_needed(), M.event,
        story_arc() + 1);
}

static void ready(void)
{
    M.state = MP_READY;
    M.t = 0;
    M.quiet = 0;
    BOT("mappa ready charged=%d\n", story_charged());
}

static bool has_wand(void) { return story_seen(SEEN_PROLOGO) || story_arc() > 0; }

/* where the gem of the monster waiting at place ch + 1 floats: above its head, or beside it when a tall
   one (the ogre on his hill) leaves no room above (top left corner of the gem) */
static void gem_pos(int ch, int fy, int *gx, int *gy)
{
    const sprite_t *s = gfx_sprite(story_foe_sprite(ch, 'c')), *g = gfx_sprite(story_ch(ch)->gem);
    int fx_ = M.loc_x[ch + 1] + 10;
    *gx = fx_ - g->w / 2;
    *gy = fy - s->h - 20;
    if (*gy < 2) {
        *gx = fx_ - s->w / 2 - g->w + 4;
        *gy = fy - s->h + 8;
    }
}

static void sparkle_details(int ch) /* the wand in her hand and the gem of the monster shine now and then */
{
    if (has_wand() && G.frame % 45 == 0) {
        int hx, hy;
        hero_hand(&G.hero, 1, &hx, &hy);
        fx_twinkle(hx + rng_range(-2, 2), hy - 17, 0);
    }
    if (ch < CH_COUNT && M.foe_shown && G.frame % 70 == 35) {
        const sprite_t *g = gfx_sprite(story_ch(ch)->gem);
        int gx, gy;
        gem_pos(ch, M.loc_y[ch + 1], &gx, &gy);
        fx_twinkle(gx + g->w / 2 + 6, gy + 2, 0);
    }
}

static void update(void)
{
    M.t++;
    int ch = story_chapter();
    sparkle_details(ch);
    switch (M.state) {
    case MP_FREE:
        M.grow = M.t * 999 / GROW;
        if (M.t % 8 == 0 && ch >= 1)
            fx_sparkles(M.reg_x[ch], M.reg_y[ch], M.reg_r[ch] * M.t / GROW, 3);
        if (M.t == GROW && ch >= 1 && story_arc() == ARC_MUSICA) { /* the note of the place plays again */
            static const char *const NOTE[4] = {"nota_do", "nota_re", "nota_mi", "nota_fa"};
            sfx(NOTE[clampi(ch - 1, 0, 3)]);
            fx_burst(M.reg_x[ch], M.reg_y[ch] - 10, 10);
        }
        if (M.t == GROW && ch >= 1 && ch < CH_COUNT) /* the friend set free comes out at its place */
            fx_sparkles(M.loc_x[ch] + 36, M.loc_y[ch] - 30, 20, 14);
        if (M.t == GROW && ch >= 1 && story_arc() == ARC_GIOCHI) { /* the toys of the place are wound up again */
            sfx("carica");
            fx_burst(M.reg_x[ch], M.reg_y[ch] - 10, 10);
        }
        if (M.t % 3 == 0 && ch >= 1 && M.t <= GROW) { /* twinkles on the edge of the colours coming back */
            float a = (float)rng_range(0, 359) * 3.14159265f / 180.0f, r = (float)(M.reg_r[ch] * M.t / GROW);
            fx_twinkle(M.reg_x[ch] + (int)(cosf(a) * r), M.reg_y[ch] + (int)(sinf(a) * r), 0);
        }
        if (M.t > GROW && !voice_busy()) {
            M.grow = 999;
            if (ch < CH_COUNT) { /* the next monster shows up */
                M.foe_shown = true;
                sfx("poof");
                fx_sparkles(M.loc_x[ch + 1], M.loc_y[ch + 1] - 30, 24, 16);
            }
            M.state = MP_SAY;
            M.t = 0;
            say_status();
        }
        break;
    case MP_SAY:
        /* the guide's words are not cut by a hurried press: held down for a second, they are (grown-ups) */
        M.hold_a = btn_held(BTN_A) ? M.hold_a + 1 : 0;
        if (M.t > 20 && !voice_busy()) {
            M.recap = false;
            ready();
        } else if (M.hold_a >= FPS && M.t > 20) {
            voice_stop();
            M.recap = false;
            ready();
        }
        break;
    case MP_READY:
        M.quiet = voice_busy() ? 0 : M.quiet + 1;
        if (M.quiet == 12 * FPS && !M.reminded) { /* distracted? once, not on and on */
            M.reminded = true;
            repeat();
        }
        if (G.hero.anim == HA_IDLE && G.hero.t > 240)
            hero_play(&G.hero, HA_WAVE, 60);
        if (story_charged() && M.foe_shown && ch < CH_COUNT && M.t % 60 == 20) /* a path of light to the monster */
            fx_trail(G.hero.x + 8, G.hero.y - 30, M.loc_x[ch + 1] + 4, M.loc_y[ch + 1] - 24, 7);
        if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            repeat();
        } else if (btn_pressed(BTN_A)) { /* (not START: held down, it opens the pause) */
            sfx("star");
            voice_stop();
            if (story_charged()) { /* off to the monster */
                M.state = MP_WALK;
                M.t = 0;
                BOT("mappa go=%s\n", story_ch(ch)->foe);
            } else {
                game_goto(SC_MENU);
            }
        }
        break;
    case MP_WALK: {
        int k = clampi(M.t * 256 / WALK, 0, 256);
        G.hero.x = M.loc_x[ch] + (M.loc_x[ch + 1] - 18 - M.loc_x[ch]) * k / 256;
        G.hero.y = M.loc_y[ch] + (M.loc_y[ch + 1] - M.loc_y[ch]) * k / 256;
        if (G.hero.anim != HA_WALK) /* 0.13: she walks, one foot after the other */
            hero_play(&G.hero, HA_WALK, 0);
        if (M.t <= WALK && (G.hero.t % 28 == 7 || G.hero.t % 28 == 21)) /* little sparkling footsteps */
            fx_twinkle(G.hero.x + (G.hero.t % 28 == 7 ? -4 : 4), G.hero.y - 1, 0);
        if (M.t > WALK + 10)
            racconto_play(R_SFIDA, SC_SFIDA);
        break;
    }
    }
}

static void draw_wand_meter(void)
{
    const sprite_t *w = gfx_sprite("bacchetta");
    gfx_blit(w, 6, 4, 0);
    int need = story_charge_needed();
    for (int i = 0; i < need; i++)
        gfx_blit(gfx_sprite(i < G.prog.charge ? "lstar_on" : "lstar_off"), 18 + i * 12, 9, 0);
}

static void draw_gems(void)
{
    int counted = -1; /* the recap: the gem being counted hops */
    if (M.recap) {
        sound_t v = voice_current();
        for (int i = 0; i < 4; i++)
            if (v >= 0 && v == snd_find(SND_VOICE, COUNT_VOICE[i]))
                counted = i;
    }
    for (int i = 0; i < CH_COUNT; i++) {
        const sprite_t *g = gfx_sprite(story_ch(i)->gem);
        int x = 206 + i * 22, y = 240 - 4 - g->h;
        if (i == counted) {
            y -= 8;
            if (G.frame % 6 == 0) {
                int dx, dy;
                rng_pair(-6, 6, -2, 8, &dx, &dy);
                fx_twinkle(x + g->w / 2 + dx, y + dy, 0);
            }
        }
        if (i < G.prog.chapter) {
            gfx_blit(g, x, y, 0);
        } else {
            gfx_set_tint(rgb565(0x60, 0x5a, 0x70), 220); /* still missing: a grey shadow */
            gfx_blit(g, x, y, 0);
            gfx_set_tint(0, 0);
        }
    }
}

static int bob_map(void) { return swing((int)G.frame, 80, 2); } /* a soft float (0.13) */

static void draw(void)
{
    int ch = story_chapter();
    const arc_t *ar = story_arc_def();
    if (ch >= CH_COUNT) {
        amb_bg(ar->map); /* every place freed: the map is alive again */
    } else {
        gfx_draw_bg(gfx_image(ar->map_off));
        const image_t *col = gfx_image(ar->map);
        for (int p = 1; p <= ch; p++) {
            int r = M.reg_r[p];
            if (p == ch && M.state == MP_FREE)
                r = r * clampi(M.grow, 0, 999) / 999;
            gfx_blit_image_circle(col, M.reg_x[p], M.reg_y[p], r);
        }
    }
    /* the friends set free stay at their places (0.12): good again, hopping; the one of the place where
       Deva stands keeps her company beside her; at the end the villain too, good */
    for (int p = 1; p <= ch && p < CH_COUNT; p++) {
        if (p == ch && M.state == MP_FREE && M.t < GROW)
            continue; /* it comes out when the colours are back */
        const char *fname = story_foe_sprite(p - 1, 'b');
        const sprite_t *f = gfx_sprite(fname);
        int x = M.loc_x[p] + (p == ch ? 36 : 10), lift, sdy, sdx; /* a happy hop now and then (0.13) */
        hop_every((int)G.frame, (uint32_t)p * 97u, 150 + p * 23, 26, 5, f->h, &lift, &sdy, &sdx);
        actor_draw(fname, x, M.loc_y[p] - lift, 1, 0, 0, sdy, sdx);
    }
    if (ch >= CH_COUNT)
        actor_draw(ar->v_good, M.loc_x[CH_COUNT] + 36, M.loc_y[CH_COUNT] - bob_map(), 1, 0, 0, 0, 0);
    /* the monster (or the villain) waiting at the next place, with its gem */
    if (ch < CH_COUNT && M.foe_shown) {
        int fx_ = M.loc_x[ch + 1] + 10, fy = M.loc_y[ch + 1] - bob_map();
        actor_draw(story_foe_sprite(ch, 'c'), fx_, fy, 1, 0, 0, 0, 0);
        int gx, gy;
        gem_pos(ch, fy, &gx, &gy);
        gfx_blit(gfx_sprite(story_ch(ch)->gem), gx, gy - swing((int)G.frame, 56, 2), 0);
    }
    hero_draw(&G.hero, &G.prog);
    if (has_wand()) { /* she has the wand */
        int hx, hy;
        hero_hand(&G.hero, 1, &hx, &hy);
        gfx_blit(gfx_sprite("bacchetta"), hx - 4, hy - 18, 0);
    }
    if (ch < CH_COUNT)
        draw_wand_meter();
    draw_gems();
    if (M.state == MP_READY && M.t > 150 && ((M.t / 30) & 1))
        gfx_blit(gfx_sprite("btn_a"), G.hero.x + 16, G.hero.y - 84, 0);
    fx_draw();
}

const scene_t SCENE_MAPPA = {enter, update, draw};
