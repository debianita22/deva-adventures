/* Deva's Awesome Adventures - "Batti il ritmo": Deva claps a short rhythm,
 * she repeats it with the red button.
 *
 * The rhythm is drawn as dots on a line, spaced by time (a first musical
 * notation: long gaps look long). Her claps appear on a second line under
 * Deva's, so the two can be compared. Only the intervals count, scaled to
 * her own tempo: a constant delay of the screen or of the sound, or a
 * slower pace, does not matter. After two misses, "facciamolo insieme": the
 * dots light up in time and she claps along.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"

#define UNIT 18         /* frames per rhythm unit (~200 bpm eighths, ~100 bpm beats) */
#define MAXB 7
#define LINE_X0 64
#define LINE_W 192
#define LINE_Y1 68      /* Deva's rhythm, under the disco ball */
#define LINE_Y2 108     /* hers */
/* 0.12: the beats at 2x (24 px; the claps are at least 32 px apart on the line), a golden note at the
   start of the line to listen to, a hand at the start of hers */
#define TOL 36          /* percent tolerance on each interval */
#define TIMEOUT (3 * FPS)

/* gaps between claps, in units */
static const signed char P1[][MAXB] = {{2, 2, 0}, {2, 2, 2, 0}};
static const signed char P2[][MAXB] = {{1, 2, 0}, {2, 1, 0}, {1, 1, 2, 0}, {2, 1, 1, 0}};
static const signed char P3[][MAXB] = {{1, 1, 2, 0}, {2, 1, 1, 0}, {1, 2, 1, 0}, {2, 2, 1, 1, 0}};
static const signed char P4[][MAXB] = {{1, 1, 1, 2, 0}, {2, 1, 1, 1, 0}, {1, 2, 1, 1, 0}, {1, 1, 2, 1, 0}};
static const signed char P5[][MAXB] = {{1, 1, 2, 1, 1, 0}, {2, 1, 1, 2, 0}, {1, 2, 2, 1, 0}, {1, 1, 1, 1, 2, 0}};

enum { R_INTRO, R_DEMO, R_INPUT, R_RIGHT, R_WRONG };

static struct {
    int state, t, level;
    int n;                 /* claps */
    int at[MAXB + 1];      /* Deva's clap times, frames from the first */
    int total;             /* frames of the whole rhythm */
    int demo_i;            /* claps played in this demo */
    int got[MAXB + 2], ngot;
    int attempts;
    bool guided;
    int question, stars;
    int flash;             /* hand burst over Deva */
} R;

static void pick_pattern(void)
{
    const signed char *p;
    switch (R.level) {
    case 1: p = P1[rng_range(0, ARRAY_LEN(P1) - 1)]; break;
    case 2: p = P2[rng_range(0, ARRAY_LEN(P2) - 1)]; break;
    case 3: p = P3[rng_range(0, ARRAY_LEN(P3) - 1)]; break;
    case 4: p = P4[rng_range(0, ARRAY_LEN(P4) - 1)]; break;
    default: p = P5[rng_range(0, ARRAY_LEN(P5) - 1)]; break;
    }
    R.at[0] = 0;
    R.n = 1;
    for (int i = 0; i < MAXB && p[i] > 0; i++) {
        R.at[R.n] = R.at[R.n - 1] + p[i] * UNIT;
        R.n++;
    }
    R.total = R.at[R.n - 1];
}

static void start_demo(bool announce)
{
    R.state = R_DEMO;
    R.t = 0;
    R.demo_i = 0;
    R.ngot = 0;
    if (announce)
        say("ritmo_ascolta");
    BOT("ritmo_show\n");
}

static void new_pattern(void)
{
    R.level = clampi(G.prog.level[GAME_RITMO], 1, 5);
    R.attempts = 0;
    R.guided = false;
    pick_pattern();
    char s[64];
    int pos = 0;
    for (int i = 0; i < R.n && pos < (int)sizeof(s) - 5; i++)
        pos += snprintf(s + pos, sizeof(s) - pos, "%s%d", i ? "," : "", R.at[i]);
    BOT("ritmo pattern=%s level=%d\n", s, R.level);
    start_demo(true);
}

static void enter(void)
{
    memset(&R, 0, sizeof(R));
    hero_init(&G.hero, 160, 208);
    music_stop(); /* claps need silence around them */
    if (!G.intro_done[GAME_RITMO]) {
        G.intro_done[GAME_RITMO] = true;
        say("ritmo_intro");
        hero_play(&G.hero, HA_WAVE, 90);
        R.state = R_INTRO;
    } else {
        new_pattern();
    }
}

static void clap(bool demo)
{
    sfx("clap");
    if (demo) { /* 0.13: Deva shows the rhythm clapping her hands in front of her */
        hero_move(&G.hero, POSE_CLAP, false, 8);
        fx_twinkle(G.hero.x - 7, G.hero.y - 24, 0);
        fx_twinkle(G.hero.x + 7, G.hero.y - 22, 1);
        return;
    }
    hero_move(&G.hero, POSE_UP, false, 8); /* hers: hands up, the big hands over her head */
    R.flash = 10;
    fx_twinkle(G.hero.x - 16, G.hero.y - 90, 0); /* the clap sparkles */
    fx_twinkle(G.hero.x + 14, G.hero.y - 84, 1);
}

static int line_x(int frames)
{
    int span = R.total > 0 ? R.total : 1;
    return LINE_X0 + frames * LINE_W / span;
}

/* her rhythm against Deva's: same number of claps, every gap within TOL
 * percent once both are scaled to the same total length */
static bool evaluate(void)
{
    if (R.ngot != R.n)
        return false;
    int mine = R.got[R.n - 1] - R.got[0];
    if (R.n < 2 || mine <= 0)
        return R.n < 2;
    if (mine * 100 < R.total * 45 || mine * 100 > R.total * 220)
        return false; /* far too fast or too slow */
    for (int i = 1; i < R.n; i++) {
        int want = (R.at[i] - R.at[i - 1]) * mine;       /* scaled to her length */
        int have = (R.got[i] - R.got[i - 1]) * R.total;
        if (absi(have - want) * 100 > want * TOL)
            return false;
    }
    return true;
}

static void log_attempt(bool ok)
{
    char target[48], chosen[48], subject[24];
    int pos = 0;
    for (int i = 1; i < R.n; i++)
        pos += snprintf(target + pos, sizeof(target) - pos, "%s%d", i > 1 ? "-" : "", (R.at[i] - R.at[i - 1]) / UNIT);
    if (R.n < 2)
        snprintf(target, sizeof(target), "1");
    pos = 0;
    chosen[0] = 0;
    for (int i = 1; i < R.ngot && pos < (int)sizeof(chosen) - 6; i++)
        pos += snprintf(chosen + pos, sizeof(chosen) - pos, "%s%d", i > 1 ? "-" : "", R.got[i] - R.got[i - 1]);
    snprintf(subject, sizeof(subject), "ritmo:%d-battiti%s", R.n, R.guided ? "-insieme" : "");
    game_log_answer("ritmo", R.level, subject, target, chosen, ok, R.attempts + 1);
    BOT("answer pick=0 ok=%d attempt=%d\n", ok, R.attempts + 1);
}

static void judge(void)
{
    bool ok = R.guided ? R.ngot >= R.n : evaluate();
    log_attempt(ok);
    if (ok) {
        static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
        sfx("ding");
        say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
        if (G.cfg.rumble)
            frontend_rumble(1, 10);
        hero_play(&G.hero, HA_DANCE, 100);
        fx_confetti(160, 120, 36);
        fx_burst(160, 120, 12);
        fx_hearts(G.hero.x + 4, G.hero.y - 70, 3);
        R.stars++;
        G.prog.stars_total++;
        int nq = G.cfg.questions_per_round;
        game_star_fly(160, 120, 160, R.stars - 1, nq); /* the star flies to its place */
        sfx("star");
        if (game_level_result(GAME_RITMO, R.attempts == 0 && !R.guided, R.guided)) {
            say_then("livello_su");
            fx_starfall(26); /* a shower of stars for the new level */
        }
        R.state = R_RIGHT;
    } else {
        sfx("boop");
        R.attempts++;
        hero_play(&G.hero, HA_OH, 45);
        if (R.attempts >= 2) {
            R.guided = true;
            say("ritmo_insieme");
            BOT("hint\n");
        } else {
            say("ritmo_ops");
        }
        R.state = R_WRONG;
    }
    R.t = 0;
    input_block(20);
}

static void update(void)
{
    R.t++;
    if (R.flash > 0)
        R.flash--;
    switch (R.state) {
    case R_INTRO:
        if (!voice_busy() && R.t > 30)
            new_pattern();
        break;
    case R_DEMO:
        if (voice_busy() && R.demo_i == 0) {
            R.t = 0; /* wait for "Ascolta!" */
            break;
        }
        while (R.demo_i < R.n && R.t - 20 == R.at[R.demo_i]) {
            clap(true);
            R.demo_i++;
        }
        if (G.hero.anim == HA_IDLE && R.demo_i < R.n) /* between the claps: her hands ready */
            hero_move(&G.hero, POSE_CLAP_OPEN, false, 0);
        if (R.t > 20 + R.total + 40) {
            hero_play(&G.hero, HA_IDLE, 0);
            say("balla_tocca"); /* "Tocca a te!" */
            R.state = R_INPUT;
            R.t = 0;
            R.ngot = 0;
            game_question_ready();
            BOT("ritmo_turn guided=%d\n", R.guided);
        }
        break;
    case R_INPUT: {
        /* together: the dots light up in time, starting when she is ready */
        if (btn_pressed(BTN_A) && R.ngot < MAXB + 2) {
            R.got[R.ngot++] = R.t;
            clap(false);
            if (R.ngot == R.n)
                judge();
            break;
        }
        if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            game_question_replay();
            start_demo(true);
            break;
        }
        if (R.ngot > 0 && R.t - R.got[R.ngot - 1] > TIMEOUT)
            judge(); /* she stopped before the end */
        else if (R.ngot == 0 && R.t == 15 * FPS)
            start_demo(true); /* distracted: listen again */
        break;
    }
    case R_WRONG:
        if (R.t > 30 && !voice_busy())
            start_demo(false);
        break;
    case R_RIGHT:
        if (R.t > 100 && !voice_busy()) {
            if (++R.question >= G.cfg.questions_per_round) {
                music_play(snd_find(SND_MUSIC, "palco"));
                game_round_done(GAME_RITMO);
            } else {
                new_pattern();
            }
        }
        break;
    }
}

static void draw_dots(int y, const int *at, int n, int lit, bool guide_blink)
{
    const sprite_t *off = gfx_sprite("beat_off"), *on = gfx_sprite("beat_on");
    for (int i = 0; i < n; i++) {
        int x = line_x(at[i]);
        bool l = i < lit;
        const sprite_t *s = l ? on : off;
        int dy = (l && i == lit - 1 && R.flash > 4) ? -3 : 0;
        if (guide_blink && !l && ((R.t / 8) & 1))
            s = on;
        gfx_blit_scaled(s, x - s->w, y - s->h + dy, 2, 0);
    }
}

static void draw(void)
{
    amb_bg("bg_palco");
    game_draw_stars(160, R.stars, G.cfg.questions_per_round);
    if (R.state != R_INTRO) {
        /* Deva's line: the whole rhythm, lit as she claps */
        gfx_fill_rect(LINE_X0 - 14, LINE_Y1 - 1, LINE_W + 28, 3, rgb565(0xcb, 0xab, 0xf2));
        const sprite_t *nt = gfx_sprite("nota_oro"), *hd = gfx_sprite("hand");
        gfx_blit(nt, LINE_X0 - 26 - nt->w / 2, LINE_Y1 - nt->h / 2, 0);
        gfx_blit_scaled(hd, LINE_X0 - 26 - hd->w, LINE_Y2 - hd->h, 2, 0);
        int lit = R.state == R_DEMO ? R.demo_i : R.n;
        bool guide = R.state == R_INPUT && R.guided;
        if (guide) { /* together: the next dot to clap blinks */
            lit = R.ngot;
        }
        draw_dots(LINE_Y1, R.at, R.n, lit, guide);
        /* her line: where her claps fell, on the same scale */
        gfx_fill_rect(LINE_X0 - 14, LINE_Y2 - 1, LINE_W + 28, 3, rgb565(0xff, 0x93, 0xc6));
        if (R.ngot > 0 && (R.state == R_INPUT || R.state == R_RIGHT || R.state == R_WRONG)) {
            int rel[MAXB + 2];
            for (int i = 0; i < R.ngot; i++) {
                int d = R.got[i] - R.got[0];
                /* her taps are drawn at her own tempo scaled to Deva's line */
                int mine = R.got[R.ngot - 1] - R.got[0];
                int span = R.state == R_INPUT ? (R.total > 0 ? R.total : 1) : (mine > 0 ? mine : 1);
                rel[i] = R.total > 0 ? d * R.total / span : 0;
                if (rel[i] > R.total * 3 / 2)
                    rel[i] = R.total * 3 / 2;
            }
            const sprite_t *h = gfx_sprite("beat_her");
            for (int i = 0; i < R.ngot; i++) {
                int x = clampi(line_x(rel[i]), 14, SCREEN_W - 14);
                gfx_blit_scaled(h, x - h->w, LINE_Y2 - h->h, 2, 0);
            }
        }
    }
    hero_draw(&G.hero, &G.prog);
    if (R.flash > 0) { /* clapping hands over her head */
        const sprite_t *hd = gfx_sprite("hand");
        gfx_blit_scaled(hd, G.hero.x - hd->w, G.hero.y - 96, 2, 0);
    }
    if (R.state == R_INPUT && R.ngot == 0 && R.t > 60 && ((R.t / 20) & 1))
        gfx_blit(gfx_sprite("btn_a"), G.hero.x + 26, G.hero.y - 60, 0);
    fx_draw();
}

const scene_t SCENE_RITMO = {enter, update, draw};
