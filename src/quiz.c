/* Deva's Awesome Adventures - shared quiz engine.
 *
 * Flow: prepare -> spoken question -> she picks a card with the D-pad and A.
 * A wrong card switches off; after the second mistake the game's guided help
 * runs and only the right card is left. Every question ends with a success
 * and a star; a full row of stars opens the dressing room. Multi-step
 * questions repeat choose -> next step until the last step is right.
 * SPDX-License-Identifier: MIT
 */
#include "quiz.h"

#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "hud.h"
#include "trans.h"
#include "ui.h"

static quiz_t Q;
static const quiz_host_t *H; /* a duel of the tale, NULL for a plain game */

static const int CARD_CX[3] = {119, 191, 263};

int quiz_card_cx(int i) { return CARD_CX[clampi(i, 0, 2)]; }
int quiz_card_cy(void) { return 202; }

int quiz_round_len(void)
{
    int n = G.cfg.questions_per_round;
    if (H)
        return H->questions;
    if (Q.def && Q.def->max_questions > 0 && Q.def->max_questions < n)
        n = Q.def->max_questions;
    return n;
}

static void bot_question(void)
{
    char v[3][32];
    for (int i = 0; i < 3; i++)
        Q.def->card_value(&Q, i, v[i], sizeof(v[i]));
    BOT("question game=%s level=%d subject=%s opts=%s,%s,%s correct=%d step=%d/%d off=%d%d%d\n", Q.def->name,
        Q.level, Q.def->subject(&Q), v[0], v[1], v[2], Q.correct, Q.step + 1, Q.steps, Q.off[0], Q.off[1],
        Q.off[2]);
}

static void reset_step(void)
{
    memset(Q.off, 0, sizeof(Q.off));
    Q.sel = 1;
    Q.attempts = 0;
    Q.idle = 0;
    Q.hint_step = 0;
    Q.hint_wait = 0;
}

static void new_question(void)
{
    if (H)
        Q.def = H->next(Q.question);
    Q.level = clampi(G.prog.level[Q.def->game], 1, 5);
    reset_step();
    Q.errors = 0;
    Q.assisted = false;
    Q.step = 0;
    Q.steps = 1;
    Q.card = Q.def->card;
    Q.gentle = false;
    Q.def->setup(&Q, Q.level);
    if (H) /* a duel: no star row, the scene keeps count */
        BOT("duel question=%d/%d game=%s\n", Q.question + 1, H->questions, Q.def->name);
    Q.steps = clampi(Q.steps, 1, 16);
    Q.state = Q_PREPARE;
    Q.t = 0;
    Q.deal = trans_active() ? -14 : 0; /* the new scene is still opening: deal a little later */
    bot_question();
}

static void panel_anchor(void)
{
    int x0 = 80, y0 = 28, x1 = 302, y1 = 166;
    gfx_anchor("panel0", &x0, &y0);
    gfx_anchor("panel1", &x1, &y1);
    Q.panel_cx = (x0 + x1) / 2;
    Q.panel_cy = (y0 + y1) / 2;
}

void quiz_enter_host(const quiz_host_t *host)
{
    memset(&Q, 0, sizeof(Q));
    H = host;
    Q.duel = true;
    panel_anchor();
    new_question();
}

void quiz_enter(const quiz_def_t *def)
{
    memset(&Q, 0, sizeof(Q));
    H = NULL;
    Q.def = def;
    panel_anchor();
    hero_init(&G.hero, 30, 214);
    music_play(snd_find(SND_MUSIC, "palco"));
    if (!G.intro_done[def->game] && def->intro) {
        G.intro_done[def->game] = true;
        say(def->intro_voice ? def->intro_voice() : def->intro);
        hero_play(&G.hero, HA_DANCE, 90);
        Q.state = Q_INTRO;
        Q.t = 0;
    } else {
        new_question();
    }
}

/* A new question: the cross moves at once, the red button waits until the cards have landed and the
   question has been asked (2.5 seconds at most). 0.14: a child mashing the red button answered before
   the cards had landed, a quarter of the answers in the first second - by chance, not by counting. */
#define LISTEN_MAX 150
#define DEAL_DONE 20

static void enter_input(void)
{
    Q.state = Q_INPUT;
    Q.t = 0;
    Q.idle = 0;
    if (!Q.listen) {
        game_question_ready();
        BOT("ready sel=%d\n", Q.sel);
    }
}

static void listen_check(void)
{
    if (Q.listen && Q.deal >= DEAL_DONE && (Q.t >= LISTEN_MAX || (Q.t >= 12 && !voice_busy()))) {
        Q.listen = false;
        game_question_ready();
        BOT("ready sel=%d\n", Q.sel);
    }
}

void quiz_select(int i)
{
    if (i < 0 || i > 2 || Q.off[i])
        return;
    if (i != Q.sel) /* a little trail of stars from card to card */
        fx_trail(quiz_card_cx(Q.sel), quiz_card_cy() - 20, quiz_card_cx(i), quiz_card_cy() - 20, 4);
    Q.sel = i;
    BOT("sel=%d\n", Q.sel);
    sfx("blip");
    Q.def->hover(&Q, i);
}

static void move_sel(int dir)
{
    for (int i = Q.sel + dir; i >= 0 && i < 3; i += dir)
        if (!Q.off[i]) {
            quiz_select(i);
            return;
        }
}

static void right_answer(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
    bool first_try = Q.errors == 0 && !Q.assisted;
    sfx("ding");
    /* a sad or scary feeling: no "evviva!" - the game (or the tale) says it softly,
       "sì, Deva è triste. Un abbraccio la aiuta!" */
    bool soft = Q.def->gentle || Q.gentle;
    if (soft)
        voice_stop();
    else if (name_voice("ok_nome") && rng_range(0, 3) == 0)
        say("ok_nome");
    else
        say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
    if (Q.def->right)
        Q.def->right(&Q);
    if (G.cfg.rumble)
        frontend_rumble(1, 10);
    if (soft) {
        fx_sparkles(quiz_card_cx(Q.correct), quiz_card_cy() - 16, 12, 8);
    } else {
        fx_confetti(quiz_card_cx(Q.correct), quiz_card_cy() - 16, 28);
        fx_burst(quiz_card_cx(Q.correct), quiz_card_cy() - 4, 12);
    }
    Q.stars++;
    G.prog.stars_total++;
    if (!H) {
        hero_play(&G.hero, soft ? HA_WAVE : HA_DANCE, 100);
        fx_hearts(G.hero.x + 4, G.hero.y - 70, soft ? 2 : 3); /* a hug, or a party */
        int n = quiz_round_len();
        game_star_fly(quiz_card_cx(Q.correct), quiz_card_cy() - 12, Q.panel_cx, Q.stars - 1, n);
        sfx("star");
    } else {
        fx_hearts(G.hero.x + 4, G.hero.y - 70, 2); /* in a duel too: Deva is happy */
    }
    /* a level up is told in the games, not in the middle of a duel of the tale,
       and not after a sad or scary feeling (no party there) */
    if (game_level_result(Q.def->game, first_try, Q.assisted) && !H && !soft) {
        say_then("livello_su");
        fx_starfall(26); /* a shower of stars for the new level */
    }
    if (H && H->right)
        H->right(&Q);
    Q.state = Q_RIGHT;
    Q.t = 0;
    input_block(30);
}

/* a step of a multi-step question is right: a small celebration, then the next step */
static void step_done(void)
{
    sfx("ding");
    fx_sparkles(quiz_card_cx(Q.correct), quiz_card_cy() - 20, 10, 6);
    fx_burst(quiz_card_cx(Q.correct), quiz_card_cy() - 4, 8);
    Q.state = Q_STEP;
    Q.t = 0;
    input_block(16);
}

static void wrong_answer(int i)
{
    sfx("boop");
    Q.off[i] = true;
    Q.wrong = i;
    fx_puff(quiz_card_cx(i), quiz_card_cy() - 30); /* poof: that one goes to sleep */
    Q.attempts++;
    Q.errors++;
    say(Q.attempts == 1 ? voice_variant("ko_1") : "ko_2");
    if (H) {
        if (H->wrong)
            H->wrong(&Q);
    } else {
        hero_play(&G.hero, HA_OH, 45);
    }
    for (int d = 1; d < 3; d++) { /* highlight the nearest card still on */
        if (i + d < 3 && !Q.off[i + d]) {
            Q.sel = i + d;
            break;
        }
        if (i - d >= 0 && !Q.off[i - d]) {
            Q.sel = i - d;
            break;
        }
    }
    Q.state = Q_WRONG;
    Q.t = 0;
    input_block(20);
}

static void choose(int i)
{
    bool ok = i == Q.correct;
    char target[32], chosen[32];
    Q.def->card_value(&Q, Q.correct, target, sizeof(target));
    Q.def->card_value(&Q, i, chosen, sizeof(chosen));
    game_log_answer(Q.def->name, Q.level, Q.def->subject(&Q), target, chosen, ok,
               Q.attempts + 1);
    BOT("answer pick=%d ok=%d attempt=%d\n", i, ok, Q.attempts + 1);
    if (ok && Q.step + 1 < Q.steps)
        step_done();
    else if (ok)
        right_answer();
    else
        wrong_answer(i);
}

#define DEAL_GAP 5 /* frames between one card and the next */
static const int8_t DEAL[] = {80, 58, 42, 29, 18, 10, 4, 0, -3, -4, -3, -2, -1}; /* y offset: up, a bounce */

static int deal_dy(int i)
{
    int k = Q.deal - i * DEAL_GAP;
    if (k < 0)
        return DEAL[0]; /* still under the edge of the screen */
    return k < (int)ARRAY_LEN(DEAL) ? DEAL[k] : 0;
}

void quiz_update(void)
{
    Q.t++;
    if (Q.deal < 1000) {
        Q.deal++;
        for (int i = 0; i < 3; i++)
            if (Q.deal == i * DEAL_GAP + 8) { /* the card lands: two twinkles on its corners */
                fx_twinkle(quiz_card_cx(i) - 24, quiz_card_cy() - 22, 0);
                fx_twinkle(quiz_card_cx(i) + 23, quiz_card_cy() - 18, 3);
            }
    }
    if (Q.def->tick)
        Q.def->tick(&Q);
    if (H && H->tick)
        H->tick(&Q);
    switch (Q.state) {
    case Q_INTRO:
        if (!voice_busy() && Q.t > 30)
            new_question();
        break;
    case Q_PREPARE:
        if (Q.def->prepare(&Q)) {
            Q.def->ask(&Q);
            /* spoken tips: arrows + A the first time, B the second */
            if (G.tutorial == 0)
                say_then("istruzioni");
            else if (G.tutorial == 1)
                say_then("aiuto_b");
            if (G.tutorial < 2)
                G.tutorial++;
            Q.listen = true;
            enter_input();
        }
        break;
    case Q_INPUT:
        listen_check();
        if (voice_busy() || btn_held(BTN_LEFT) || btn_held(BTN_RIGHT) || btn_held(BTN_UP) || btn_held(BTN_DOWN))
            Q.idle = 0;
        else if (++Q.idle == 15 * FPS)
            Q.def->ask(&Q); /* distracted? ask again, once */
        if (Q.def->key && Q.def->key(&Q))
            ; /* the game used the cross */
        else if (btn_pressed(BTN_LEFT))
            move_sel(-1);
        else if (btn_pressed(BTN_RIGHT))
            move_sel(1);
        else if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            game_question_replay();
            Q.def->ask(&Q);
        }
        else if (btn_pressed(BTN_A) && !Q.off[Q.sel] && !Q.listen)
            choose(Q.sel);
        break;
    case Q_RIGHT:
        if (Q.t > 100 && !voice_busy()) {
            if (++Q.question >= quiz_round_len()) {
                if (H)
                    H->done();
                else
                    game_round_done(Q.def->game);
            } else {
                new_question();
            }
        }
        break;
    case Q_STEP:
        if (Q.t > 28) {
            Q.step++;
            reset_step();
            if (Q.def->next_step)
                Q.def->next_step(&Q);
            if (Q.off[Q.sel]) /* the highlight never rests on a card that is gone */
                for (int i = 0; i < 3; i++)
                    if (!Q.off[i]) {
                        Q.sel = i;
                        break;
                    }
            bot_question();
            Q.def->ask(&Q);
            enter_input();
        }
        break;
    case Q_WRONG:
        if (Q.t > 30 && !voice_busy()) {
            if (Q.attempts >= 2) {
                Q.assisted = true;
                Q.hint_step = 0;
                Q.hint_wait = 8;
                Q.state = Q_HINT;
                Q.t = 0;
                BOT("hint\n");
            } else {
                enter_input();
            }
        }
        break;
    case Q_HINT:
        if (voice_busy())
            break;
        if (Q.hint_wait > 0) {
            Q.hint_wait--;
            break;
        }
        if (Q.def->hint(&Q)) {
            Q.sel = Q.correct;
            enter_input();
        } else {
            Q.hint_step++;
            if (Q.hint_wait < 5) /* the game may ask for a longer pause */
                Q.hint_wait = 5;
        }
        break;
    }
}

/* a card, squashed or stretched around its middle (its border stays sharp): (x, y) its top left */
static void card_blit(const char *name, int x, int y, int sdy, int sdx)
{
    const sprite_t *s = gfx_sprite(name);
    gfx_blit_squash(s, x + s->w / 2, y + s->h, 1, 0, s->h / 2, sdy, sdx);
}

static void draw_cards(void)
{
    static const int8_t SHAKE[] = {3, 3, -3, -3, 3, 2, -2, -2, 2, 1, -1, -1, 1, 0}; /* "no, not me" */
    bool picture = Q.card == QUIZ_CARD_PICTURE;
    int w = picture ? 52 : 56, h = picture ? 52 : 48;
    bool selectable = Q.state == Q_INPUT || Q.state == Q_WRONG;
    const char *base = picture ? "rcard" : "card";
    char name[16];
    for (int i = 0; i < 3; i++) {
        int dy = deal_dy(i), dx = 0, sdy = 0, sdx = 0;
        /* 0.13: flying in it is a little longer, landing it squashes and springs back */
        int k = Q.deal - i * DEAL_GAP;
        if (k >= 0 && k < 6)
            sdy = 3;
        else if (k >= 7 && k < 11)
            sdy = -(11 - k), sdx = 11 - k;
        if (Q.state == Q_WRONG && i == Q.wrong && Q.t < (int)ARRAY_LEN(SHAKE)) {
            dx = SHAKE[Q.t];
            sdy = -2; /* it shrinks a little: not me */
        }
        int cx = quiz_card_cx(i) + dx, cy = quiz_card_cy() + dy;
        int x = cx - w / 2, y = cy - h / 2;
        int style = CARD_NORMAL, bob = 0;
        if (Q.off[i]) {
            style = CARD_OFF;
            snprintf(name, sizeof(name), "%s_off", base);
            card_blit(name, x, y, sdy, sdx);
        } else if ((selectable && i == Q.sel) || ((Q.state == Q_RIGHT || Q.state == Q_STEP) && i == Q.correct)) {
            style = CARD_SELECTED;
            if (Q.state != Q_INPUT && Q.state != Q_WRONG) { /* the right one: it pops, then hops for joy */
                int lift, hdy, hdx;
                hop_shape(Q.t % 26, 18, 7, 26, &lift, &hdy, &hdx);
                int pop = Q.t < 12 ? swing(Q.t, 24, 8) : 0;
                bob = -lift;
                sdy += hdy + pop;
                sdx += hdx + pop;
            } else { /* the highlighted one floats softly */
                bob = -swing((int)G.frame, 60, 2);
            }
            snprintf(name, sizeof(name), "%s_sel", base);
            card_blit(name, x - 2, y - 2 + bob, sdy, sdx);
            if (dy == 0)
                ui_glow(x - 3, y - 3 + bob - sdy, w + 5, h + 5 + sdy);
        } else {
            card_blit(base, x, y, sdy, sdx);
        }
        Q.def->card_draw(&Q, i, cx, cy + bob - sdy / 2, style);
    }
}

void quiz_draw(void)
{
    if (H) {
        H->draw_back(&Q);
    } else {
        amb_bg(Q.def->bg);
        game_draw_stars(Q.panel_cx, Q.stars, quiz_round_len());
    }
    if (Q.state != Q_INTRO) { /* the intro is Deva's dance on an empty stage */
        Q.def->draw(&Q);
        draw_cards();
    } else if (Q.def->intro_draw) { /* ...or on the game's own set, without cards */
        Q.def->intro_draw(&Q);
    }
    if (!H)
        hero_draw(&G.hero, &G.prog);
    if (Q.state == Q_INPUT && Q.t > 240 && ((Q.t / 30) & 1)) {
        /* nudge: blink the red A button next to the highlighted card */
        gfx_blit(gfx_sprite("btn_a"), quiz_card_cx(Q.sel) + 20, quiz_card_cy() - 34, 0);
    }
    /* spoken tutorial: the console, left of the panel, shows what to press */
    int hl = H ? HUD_NONE : hud_tutorial_highlight();
    if (hl != HUD_NONE)
        hud_console(0, 58, hl, ((G.frame / 10) & 1) == 0);
    fx_draw();
    if (H && H->draw_front)
        H->draw_front(&Q);
}
