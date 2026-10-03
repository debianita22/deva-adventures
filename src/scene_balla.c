/* Deva's Awesome Adventures - "Balla con me": Deva shows a short dance, one
 * move per arrow (up = arms up, down = crouch, left/right = step), each with
 * its own note; then she repeats it with the D-pad.
 *
 * The arrows stay visible in a strip (reading a row of symbols left to right),
 * from level 4 they hide once her turn starts (memory). A mistake replays the
 * dance; after two, "facciamolo insieme": the next arrow blinks until pressed.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "hud.h"

#define MAX_MOVES 6
#define MOVE_FRAMES 42 /* demo tempo: ~85 bpm */
#define SLOT 30
#define SLOT_STEP 36
#define STRIP_Y 60 /* under the disco ball */
#define FLASH_Y 122 /* centre of the big arrow over Deva's head */
#define DPAD_X 36   /* the cross close-up sits on the left, like on the console */
#define DPAD_Y 160
#define HERO_X 160
#define HERO_FEET 208

enum { MV_SU, MV_GIU, MV_SX, MV_DX, MV_COUNT };

static const struct {
    const char *arrow, *tone, *id;
    int btn;
    hero_pose_t pose;
    bool flip;
} MOVES[MV_COUNT] = {
    {"arrow_up", "tone_su", "su", BTN_UP, POSE_UP, false},
    {"arrow_down", "tone_giu", "giu", BTN_DOWN, POSE_GIU, false},
    {"arrow_left", "tone_sx", "sx", BTN_LEFT, POSE_STEP, false}, /* the step pose reaches to the left */
    {"arrow_right", "tone_dx", "dx", BTN_RIGHT, POSE_STEP, true},
};

static const struct {
    int len;
    bool hidden;
} LEVELS[5] = {{2, false}, {3, false}, {4, false}, {3, true}, {4, true}};

enum { B_INTRO, B_SHOW, B_INPUT, B_WRONG, B_RIGHT };

static struct {
    int state, t;
    int level, len;
    bool hidden, guided;
    int seq[MAX_MOVES];
    int demo_i;       /* move being shown, -1 = none */
    int idx;          /* next move she has to do */
    int pressed[MAX_MOVES], npressed;
    int flash_mv, flash_t; /* big arrow over Deva */
    int attempts;
    int question, stars;
    int idle;
    bool replayed_idle;
} B;

static void seq_string(const int *moves, int n, char *buf, size_t size)
{
    size_t pos = 0;
    buf[0] = 0;
    for (int i = 0; i < n && pos < size; i++)
        pos += (size_t)snprintf(buf + pos, size - pos, "%s%s", i ? "-" : "", MOVES[moves[i]].id);
}

static void start_show(bool announce)
{
    B.state = B_SHOW;
    B.t = 0;
    B.demo_i = -1;
    B.idx = 0;
    B.npressed = 0;
    if (announce)
        say("balla_guarda");
    BOT("balla_show\n");
}

static void new_sequence(void)
{
    B.level = clampi(G.prog.level[GAME_BALLA], 1, 5);
    B.len = LEVELS[B.level - 1].len;
    B.hidden = LEVELS[B.level - 1].hidden;
    B.guided = false;
    B.attempts = 0;
    B.replayed_idle = false;
    /* no move three times in a row; the first two moves always differ */
    for (int i = 0; i < B.len; i++) {
        int m;
        do
            m = rng_range(0, MV_COUNT - 1);
        while ((i == 1 && m == B.seq[0]) || (i >= 2 && m == B.seq[i - 1] && m == B.seq[i - 2]));
        B.seq[i] = m;
    }
    char s[64];
    seq_string(B.seq, B.len, s, sizeof(s));
    BOT("balla seq=%s level=%d hidden=%d\n", s, B.level, B.hidden);
    start_show(true);
}

static void enter(void)
{
    memset(&B, 0, sizeof(B));
    B.flash_mv = -1;
    hero_init(&G.hero, HERO_X, HERO_FEET);
    music_play(snd_find(SND_MUSIC, "palco"));
    if (!G.intro_done[GAME_BALLA]) {
        G.intro_done[GAME_BALLA] = true;
        say("balla_intro");
        hero_play(&G.hero, HA_DANCE, 120);
        B.state = B_INTRO;
    } else {
        new_sequence();
    }
}

static void do_move(int m, int hold)
{
    hero_move(&G.hero, MOVES[m].pose, MOVES[m].flip, hold);
    sfx(MOVES[m].tone);
    B.flash_mv = m;
    B.flash_t = 0;
}

static void log_attempt(bool ok)
{
    char target[64], chosen[64], subject[32];
    seq_string(B.seq, B.len, target, sizeof(target));
    seq_string(B.pressed, B.npressed, chosen, sizeof(chosen));
    snprintf(subject, sizeof(subject), "%s%d-passi%s", B.hidden ? "memoria:" : "vedi:", B.len, B.guided ? "-insieme" : "");
    game_log_answer("balla", B.level, subject, target, chosen, ok, B.attempts + 1);
    BOT("answer pick=%d ok=%d attempt=%d\n", B.npressed - 1, ok, B.attempts + 1);
}

static void success(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
    bool first_try = B.attempts == 0 && !B.guided;
    log_attempt(true);
    sfx("ding");
    if (name_voice("ok_nome") && rng_range(0, 3) == 0)
        say("ok_nome");
    else
        say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
    if (G.cfg.rumble)
        frontend_rumble(1, 10);
    hero_play(&G.hero, HA_DANCE, 100);
    fx_confetti(HERO_X, HERO_FEET - 70, 40);
    fx_burst(HERO_X, HERO_FEET - 40, 12);
    fx_hearts(HERO_X + 4, HERO_FEET - 70, 3);
    B.stars++;
    G.prog.stars_total++;
    int n = G.cfg.questions_per_round;
    game_star_fly(HERO_X, HERO_FEET - 70, 160, B.stars - 1, n); /* the star flies to its place */
    sfx("star");
    if (game_level_result(GAME_BALLA, first_try, B.guided)) {
        say_then("livello_su");
        fx_starfall(26); /* a shower of stars for the new level */
    }
    B.state = B_RIGHT;
    B.t = 0;
    input_block(30);
}

static void mistake(void)
{
    sfx("boop");
    log_attempt(false);
    B.attempts++;
    hero_play(&G.hero, HA_OH, 45);
    B.state = B_WRONG;
    B.t = 0;
    input_block(20);
    if (B.attempts >= 2) {
        B.guided = true;
        say("balla_insieme"); /* "Facciamolo insieme: segui la freccia!" */
        BOT("hint\n");
    } else {
        say("balla_ops"); /* "Ops! Guarda di nuovo." */
    }
}

static void start_input(void)
{
    B.state = B_INPUT;
    B.t = 0;
    B.idx = 0;
    B.npressed = 0;
    B.idle = 0;
    game_question_ready();
    BOT("balla_turn guided=%d\n", B.guided);
}

static int slot_x(int i);

static void update(void)
{
    B.t++;
    B.flash_t++;
    switch (B.state) {
    case B_INTRO:
        if (!voice_busy() && B.t > 30)
            new_sequence();
        break;
    case B_SHOW: {
        if (voice_busy() && B.demo_i < 0) {
            B.t = 0; /* wait for "Guarda!" */
            break;
        }
        int i = (B.t - 20) / MOVE_FRAMES;
        if (B.t >= 20 && (B.t - 20) % MOVE_FRAMES == 0 && i < B.len) {
            B.demo_i = i;
            do_move(B.seq[i], MOVE_FRAMES - 8);
        }
        if (B.t >= 20 + B.len * MOVE_FRAMES + 10) {
            B.demo_i = -1;
            say("balla_tocca"); /* "Tocca a te!" */
            start_input();
        }
        break;
    }
    case B_INPUT: {
        if (voice_busy())
            B.idle = 0;
        else if (++B.idle == 15 * FPS && !B.replayed_idle) {
            B.replayed_idle = true; /* distracted? show it once more */
            start_show(true);
            break;
        }
        if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            game_question_replay();
            start_show(true);
            break;
        }
        for (int m = 0; m < MV_COUNT; m++) {
            if (!btn_pressed(MOVES[m].btn))
                continue;
            B.idle = 0;
            bool ok = m == B.seq[B.idx];
            BOT("balla_press move=%s ok=%d\n", MOVES[m].id, ok);
            if (!ok && B.guided) {
                sfx("blip"); /* together: only the blinking arrow counts */
                break;
            }
            do_move(m, 26);
            if (B.npressed < MAX_MOVES)
                B.pressed[B.npressed++] = m;
            if (ok) { /* the step just danced twinkles on the strip */
                fx_twinkle(slot_x(B.idx) + 2, STRIP_Y + 2, 0);
                fx_twinkle(slot_x(B.idx) + SLOT - 2, STRIP_Y + SLOT - 3, 2);
            }
            if (!ok) {
                mistake();
            } else if (++B.idx == B.len) {
                success();
            }
            break;
        }
        break;
    }
    case B_WRONG:
        if (B.t > 30 && !voice_busy()) {
            if (B.guided)
                start_input();
            else
                start_show(false);
        }
        break;
    case B_RIGHT:
        if (B.t > 100 && !voice_busy()) {
            if (++B.question >= G.cfg.questions_per_round)
                game_round_done(GAME_BALLA);
            else
                new_sequence();
        }
        break;
    }
}

static int slot_x(int i) { return 160 - (B.len * SLOT_STEP - (SLOT_STEP - SLOT)) / 2 + i * SLOT_STEP; }

static void draw_strip(void)
{
    for (int i = 0; i < B.len; i++) {
        int x = slot_x(i), y = STRIP_Y;
        bool done = B.state == B_INPUT ? i < B.idx : (B.state == B_RIGHT);
        bool current = (B.state == B_SHOW && i == B.demo_i) || (B.state == B_INPUT && i == B.idx);
        bool show_arrow = true;
        if (B.state == B_SHOW)
            show_arrow = i <= B.demo_i || !B.hidden; /* hidden levels: revealed as danced */
        else if (B.state == B_INPUT && B.hidden && !B.guided)
            show_arrow = i < B.idx;
        int dy = 0;
        if (current && B.state == B_SHOW && B.t % MOVE_FRAMES < 10)
            dy = -3;
        if (B.state == B_RIGHT && B.t < 90)
            dy = ((B.t / 4 + B.len - i) % 8) < 2 ? -3 : 0;
        bool blink = B.state == B_INPUT && current && (B.guided || (!B.hidden && B.t > 8 * FPS));
        if (current || done || blink) {
            if (!blink || ((B.t / 12) & 1))
                gfx_blit(gfx_sprite("scard_sel"), x - 2, y - 2 + dy, 0);
            else
                gfx_blit(gfx_sprite("scard"), x, y + dy, 0);
        } else {
            gfx_blit(gfx_sprite("scard"), x, y + dy, 0);
        }
        if (show_arrow) {
            const sprite_t *a = gfx_sprite(MOVES[B.seq[i]].arrow);
            gfx_blit(a, x + (SLOT - a->w) / 2, y + (SLOT - a->h) / 2 + dy, 0);
        } else {
            const sprite_t *qm = gfx_sprite("qmark");
            gfx_blit(qm, x + (SLOT - qm->w) / 2, y + (SLOT - qm->h) / 2 + dy, 0);
        }
    }
}

static void draw(void)
{
    amb_bg("bg_palco");
    game_draw_stars(160, B.stars, G.cfg.questions_per_round);
    if (B.state != B_INTRO)
        draw_strip();
    hero_draw(&G.hero, &G.prog);
    /* the cross close-up: the arm of the move just danced (or pressed) lights
       up in the colour of its arrow; together, the next one blinks */
    if (B.state != B_INTRO) {
        int dir = DIR_NONE; /* MV_SU..MV_DX follow DIR_UP..DIR_RIGHT */
        if (B.flash_mv >= 0 && B.flash_t < 24)
            dir = B.flash_mv;
        else if (B.state == B_INPUT && B.guided && ((B.t / 12) & 1))
            dir = B.seq[B.idx];
        hud_dpad(DPAD_X, DPAD_Y, dir);
    }
    /* the move just danced, big over her head */
    if (B.flash_mv >= 0 && B.flash_t < 30 && B.state != B_INTRO) {
        const sprite_t *a = gfx_sprite(MOVES[B.flash_mv].arrow);
        int k = B.flash_t < 3 ? 1 : 2;
        gfx_blit_scaled(a, HERO_X - a->w * k / 2, FLASH_Y - a->h * k / 2, k, 0);
    }
    fx_draw();
}

const scene_t SCENE_BALLA = {enter, update, draw};
