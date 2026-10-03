/* Deva's Awesome Adventures - "Ginnastica": the coach calls a routine ("Due
 * salti! Una capriola! E poi il saluto!"), she does it with the buttons and
 * Deva does it on the mat. The red button jumps, down rolls, left/right turn a
 * cartwheel, up is the gymnast's salute that ends every routine.
 *
 * It trains counting actions (stopping at the right number: the voice counts
 * with her) and following spoken instructions of two or three parts. The
 * board shows the routine as little Devas in the poses, with the number of
 * times; from level 4 the cards turn over once her turn starts (memory).
 * Levels: 1 jumps 1-3, with dots; 2 one move 3-6 times; 3 two moves; 4 two
 * moves from memory; 5 three moves from memory.
 * A mistake (one jump too many, the salute too early, another move) calls the
 * routine again; after two, "facciamolo insieme": the button to press blinks.
 *
 * QUIZ_SALTI, for the duels of the tale: the monster bounces on a trampoline;
 * how many jumps did it do?
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "hud.h"
#include "quiz.h"
#include "story.h"
#include "ui.h"

enum { MV_SALTO, MV_CAPRIOLA, MV_RUOTA, MV_SALUTO, MV_COUNT };

static const struct {
    char code;       /* BOT and parent log */
    const char *id;  /* voices gi_<id>_<n>, gi_tasto_<id> */
    int frames;      /* length of the move on the mat */
    const char *sfx;
} MOVES[MV_COUNT] = {
    {'S', "salto", 30, "boing"},
    {'C', "capriola", 38, "whoosh"},
    {'R', "ruota", 38, "whoosh"},
    {'U', "saluto", 44, "star"},
};

#define MAX_PARTS 4
#define MAX_ACTS 16
#define HERO_X 160
#define FLOOR 204
#define X_MIN 70
#define X_MAX 250
#define OFF_W 64
#define OFF_H 80
#define BOARD_Y 24
#define CARD_W 50
#define CARD_H 76
#define CARD_STEP 56
#define ROLL_PIVOT 25 /* the middle of the curled-up body, above her feet */
#define DPAD_X 266
#define DPAD_Y 116

enum { GI_INTRO, GI_LISTEN, GI_INPUT, GI_FINISH, GI_WRONG, GI_RIGHT };

typedef struct {
    int move, count;
} part_t;

static struct {
    int state, t, level;
    part_t part[MAX_PARTS];
    int nparts;
    int seq[MAX_ACTS], part_of[MAX_ACTS], len; /* the routine, action by action */
    int idx;                                   /* next action she has to do */
    bool hidden, dots, guided;
    int attempts, question, stars;
    int shown;               /* parts on the board while the coach calls them */
    sound_t clip[MAX_PARTS];
    int idle;
    bool replayed;
    int last_sig;            /* the previous routine, not twice in a row */
    /* Deva on the mat */
    int x;                   /* 1/16 px */
    int act, act_t, act_dir, act_x0;
    int queue[MAX_ACTS], qdir[MAX_ACTS], nq;
    int pressed[MAX_ACTS], npressed;
    int flash, flash_t;      /* the control just used, lit on the legend */
    int intro_k;             /* intro: move being shown */
    sound_t intro_clip[MV_COUNT];
} Y;

static uint16_t s_off[OFF_W * OFF_H]; /* Deva drawn here to be turned */

/* ------------------------------------------------------------------ Deva, drawn in a pose and turned */

static hero_t posed(hero_pose_t pose, bool flip)
{
    hero_t h = G.hero;
    h.anim = HA_MOVE;
    h.move = pose;
    h.move_flip = flip;
    h.t = 1000; /* past the little hop of the jump pose */
    h.dur = 0;
    h.talking = false;
    h.expr = EXPR_NONE;
    return h;
}

/* Deva in a pose, turned by deg around a point pivot_h px above her feet,
 * that point landing on (cx, cy). */
static void draw_turned(hero_t h, int pivot_h, int cx, int cy, int deg)
{
    gfx_target(s_off, OFF_W, OFF_H);
    gfx_fill_rect(0, 0, OFF_W, OFF_H, GFX_KEY);
    h.x = OFF_W / 2;
    h.y = OFF_H - 4;
    h.no_shadow = true;
    h.mirror = false;
    hero_draw(&h, &G.prog);
    gfx_target(NULL, 0, 0);
    gfx_blit_rotated(s_off, OFF_W, OFF_H, GFX_KEY, OFF_W / 2, OFF_H - 4 - pivot_h, cx, cy, deg);
}

static void shadow_at(int x, int y, int shrink)
{
    const sprite_t *sh = gfx_sprite("shadow");
    int w = sh->w - shrink;
    if (w < 8)
        w = 8;
    gfx_set_clip(x - w / 2, 0, x + w / 2, SCREEN_H);
    gfx_blit(sh, x - sh->w / 2, y - 2, 0);
    gfx_reset_state();
}

/* ------------------------------------------------------------------ the routine */

static const char *move_clip(int move, int count, char *buf, size_t n)
{
    if (move == MV_SALUTO)
        snprintf(buf, n, "gi_saluto");
    else
        snprintf(buf, n, "gi_%s_%d", MOVES[move].id, count);
    return buf;
}

static void routine_string(const int *acts, int n, char *buf, size_t size)
{
    size_t pos = 0;
    buf[0] = 0;
    for (int i = 0; i < n && pos + 2 < size; i++) {
        buf[pos++] = MOVES[acts[i]].code;
        if (i + 1 < n)
            buf[pos++] = ',';
        buf[pos] = 0;
    }
}

static void add_part(int move, int count)
{
    if (Y.nparts < MAX_PARTS)
        Y.part[Y.nparts++] = (part_t){move, count};
}

static int signature(void)
{
    int s = 0;
    for (int i = 0; i < Y.nparts; i++)
        s = s * 31 + Y.part[i].move * 8 + Y.part[i].count;
    return s;
}

static void make_routine(void)
{
    int L = Y.level;
    Y.nparts = 0;
    if (L == 1) {
        add_part(MV_SALTO, rng_range(1, 3));
    } else if (L == 2) {
        if (rng_range(0, 2) == 0) {
            int m = rng_range(MV_CAPRIOLA, MV_RUOTA);
            add_part(m, rng_range(2, 3));
        } else {
            add_part(MV_SALTO, rng_range(3, 6));
        }
    } else {
        int moves[3] = {MV_SALTO, MV_CAPRIOLA, MV_RUOTA};
        for (int i = 2; i > 0; i--) {
            int j = rng_range(0, i), t = moves[i];
            moves[i] = moves[j];
            moves[j] = t;
        }
        int n = L == 5 ? 3 : 2;
        for (int i = 0; i < n; i++)
            add_part(moves[i], moves[i] == MV_SALTO ? rng_range(1, L >= 4 ? 3 : 2) : rng_range(1, 2));
    }
    add_part(MV_SALUTO, 1);
}

static void new_routine(void)
{
    Y.level = clampi(G.prog.level[GAME_GINNASTICA], 1, 5);
    for (int tries = 0; tries < 8; tries++) {
        make_routine();
        if (signature() != Y.last_sig)
            break;
    }
    Y.last_sig = signature();
    Y.len = 0;
    for (int p = 0; p < Y.nparts; p++)
        for (int k = 0; k < Y.part[p].count && Y.len < MAX_ACTS; k++) {
            Y.part_of[Y.len] = p;
            Y.seq[Y.len++] = Y.part[p].move;
        }
    Y.hidden = Y.level >= 4;
    Y.dots = Y.level == 1;
    Y.guided = false;
    Y.attempts = 0;
    Y.replayed = false;
    char s[48];
    routine_string(Y.seq, Y.len, s, sizeof(s));
    BOT("ginnastica seq=%s level=%d hidden=%d\n", s, Y.level, Y.hidden);
}

static void start_listen(bool announce)
{
    Y.state = GI_LISTEN;
    Y.t = 0;
    Y.shown = 0;
    Y.idx = 0;
    Y.npressed = 0;
    char id[32];
    if (announce)
        say("gi_ascolta");
    for (int p = 0; p < Y.nparts; p++) {
        move_clip(Y.part[p].move, Y.part[p].count, id, sizeof(id));
        Y.clip[p] = snd_find(SND_VOICE, id);
        if (announce || p > 0)
            say_then(id);
        else
            say(id);
    }
    BOT("ginnastica_listen\n");
}

static void start_input(void)
{
    Y.state = GI_INPUT;
    Y.t = 0;
    Y.idx = 0;
    Y.npressed = 0;
    Y.idle = 0;
    game_question_ready();
    BOT("ginnastica_turn guided=%d\n", Y.guided);
}

/* ------------------------------------------------------------------ moves on the mat */

static int travel(int move, int dir)
{
    int px = Y.x / 16, d = move == MV_CAPRIOLA ? 26 : (move == MV_RUOTA ? 44 : 0);
    int to = clampi(px + d * dir, X_MIN, X_MAX);
    return to - px;
}

static void start_act(int m, int dir)
{
    if (m == MV_CAPRIOLA && !dir) /* rolls towards the middle of the mat */
        dir = Y.x / 16 > HERO_X ? -1 : 1;
    Y.act = m;
    Y.act_t = 0;
    Y.act_dir = dir ? dir : 1;
    Y.act_x0 = Y.x;
    sfx(MOVES[m].sfx);
    if (m == MV_SALUTO)
        fx_sparkles(Y.x / 16, FLOOR - 64, 14, 10);
}

static void queue_act(int m, int dir)
{
    if (Y.act < 0) {
        start_act(m, dir);
    } else if (Y.nq < MAX_ACTS) {
        Y.queue[Y.nq] = m;
        Y.qdir[Y.nq++] = dir;
    }
}

static bool acting(void) { return Y.act >= 0 || Y.nq > 0; }

static void update_act(void)
{
    if (Y.act < 0)
        return;
    int n = MOVES[Y.act].frames;
    Y.act_t++;
    int d = travel(Y.act, Y.act_dir);
    if (Y.act == MV_CAPRIOLA || Y.act == MV_RUOTA) {
        int k = clampi(Y.act_t, 0, n);
        Y.x = Y.act_x0 + d * 16 * k / n;
    }
    if (Y.act_t >= n) {
        if (Y.act == MV_CAPRIOLA || Y.act == MV_RUOTA)
            Y.x = Y.act_x0 + d * 16;
        Y.act = -1;
        if (Y.nq > 0) {
            int m = Y.queue[0], dir = Y.qdir[0];
            memmove(Y.queue, Y.queue + 1, (size_t)(Y.nq - 1) * sizeof(int));
            memmove(Y.qdir, Y.qdir + 1, (size_t)(Y.nq - 1) * sizeof(int));
            Y.nq--;
            start_act(m, dir);
        }
    }
}

/* back to the middle of the mat, walking */
static void walk_home(void)
{
    int target = HERO_X * 16, dx = target - Y.x;
    if (dx == 0 || acting())
        return;
    int step = dx > 0 ? 24 : -24;
    if (absi(dx) < 24)
        step = dx;
    Y.x += step;
    if ((G.frame / 8) & 1)
        hero_move(&G.hero, POSE_STEP, step < 0, 8);
}

static void draw_deva(void)
{
    int x = Y.x / 16;
    if (Y.act < 0) {
        G.hero.x = x;
        G.hero.y = FLOOR;
        hero_draw(&G.hero, &G.prog);
        return;
    }
    int n = MOVES[Y.act].frames, k = clampi(Y.act_t, 0, n);
    switch (Y.act) {
    case MV_SALTO: { /* up in the air: arms and legs open */
        int lift = 36 * 4 * k * (n - k) / (n * n);
        shadow_at(x, FLOOR, lift / 3);
        hero_t h = posed(POSE_JUMP, false);
        h.x = x;
        h.y = FLOOR - lift;
        h.no_shadow = true;
        hero_draw(&h, &G.prog);
        break;
    }
    case MV_CAPRIOLA: { /* crouch, roll over, crouch */
        shadow_at(x, FLOOR, 0);
        int in = 7, out = 7, spin = n - in - out;
        if (k < in || k >= n - out) {
            hero_t h = posed(POSE_GIU, Y.act_dir < 0);
            h.x = x;
            h.y = FLOOR;
            h.no_shadow = true;
            hero_draw(&h, &G.prog);
        } else {
            int deg = 360 * (k - in) / spin * Y.act_dir;
            draw_turned(posed(POSE_GIU, Y.act_dir < 0), ROLL_PIVOT, x, FLOOR - ROLL_PIVOT, deg);
        }
        break;
    }
    case MV_RUOTA: { /* the star turns over once, sideways */
        shadow_at(x, FLOOR, 0);
        int deg = 360 * k / n * Y.act_dir;
        int rise = 8 * 4 * k * (n - k) / (n * n);
        draw_turned(posed(POSE_JUMP, false), 30, x, FLOOR - 30 - rise, deg);
        break;
    }
    default: { /* the salute: arms up, ta-da */
        hero_t h = posed(POSE_UP, false);
        h.x = x;
        h.y = FLOOR - ((k / 6) % 2 == 0 && k < 18 ? 2 : 0);
        hero_draw(&h, &G.prog);
        break;
    }
    }
}

/* ------------------------------------------------------------------ flow */

static void log_attempt(bool ok)
{
    char target[48], chosen[48], subject[64];
    routine_string(Y.seq, Y.len, target, sizeof(target));
    routine_string(Y.pressed, Y.npressed, chosen, sizeof(chosen));
    size_t pos = (size_t)snprintf(subject, sizeof(subject), "%s", Y.hidden ? "memoria:" : "vedi:");
    for (int p = 0; p < Y.nparts - 1 && pos < sizeof(subject); p++)
        pos += (size_t)snprintf(subject + pos, sizeof(subject) - pos, "%s%s%d", p ? "+" : "", MOVES[Y.part[p].move].id,
                                Y.part[p].count);
    if (Y.guided && pos < sizeof(subject))
        snprintf(subject + pos, sizeof(subject) - pos, "-insieme");
    game_log_answer("ginnastica", Y.level, subject, target, chosen, ok, Y.attempts + 1);
    BOT("answer pick=%d ok=%d attempt=%d\n", Y.npressed - 1, ok, Y.attempts + 1);
}

static void success(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
    bool first_try = Y.attempts == 0 && !Y.guided;
    log_attempt(true);
    sfx("fanfare");
    if (name_voice("ok_nome") && rng_range(0, 3) == 0)
        say("ok_nome");
    else
        say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
    if (G.cfg.rumble)
        frontend_rumble(1, 10);
    fx_confetti(Y.x / 16, FLOOR - 70, 40);
    fx_burst(Y.x / 16, FLOOR - 40, 12);
    fx_hearts(Y.x / 16 + 4, FLOOR - 70, 3);
    Y.stars++;
    G.prog.stars_total++;
    int n = G.cfg.questions_per_round;
    game_star_fly(Y.x / 16, FLOOR - 70, 160, Y.stars - 1, n); /* the star flies to its place */
    if (game_level_result(GAME_GINNASTICA, first_try, Y.guided)) {
        say_then("livello_su");
        fx_starfall(26); /* a shower of stars for the new level */
    }
    hero_play(&G.hero, HA_DANCE, 100);
    Y.state = GI_RIGHT;
    Y.t = 0;
    input_block(30);
}

static void mistake(int m)
{
    const char *why = "gi_sbagliato";
    if (m == MV_SALUTO)
        why = "gi_pochi"; /* saluted before the end */
    else if (Y.idx > 0 && m == Y.seq[Y.idx - 1])
        why = "gi_troppi"; /* one more of the same */
    log_attempt(false);
    sfx("boop");
    Y.attempts++;
    Y.nq = 0;
    Y.state = GI_WRONG;
    Y.t = 0;
    input_block(20);
    if (Y.attempts >= 2) {
        Y.guided = true;
        say(why);
        say_then("gi_insieme"); /* "Facciamolo insieme: premi il tasto che si illumina!" */
        BOT("hint\n");
    } else {
        say(why);
    }
}

static int pressed_move(int *dir)
{
    *dir = 0;
    if (btn_pressed(BTN_A))
        return MV_SALTO;
    if (btn_pressed(BTN_DOWN))
        return MV_CAPRIOLA;
    if (btn_pressed(BTN_LEFT)) {
        *dir = -1;
        return MV_RUOTA;
    }
    if (btn_pressed(BTN_RIGHT)) {
        *dir = 1;
        return MV_RUOTA;
    }
    if (btn_pressed(BTN_UP))
        return MV_SALUTO;
    return -1;
}

static void flash_control(int m, int dir)
{
    Y.flash = m == MV_SALTO ? 4 : (m == MV_CAPRIOLA ? DIR_DOWN : (m == MV_SALUTO ? DIR_UP : (dir < 0 ? DIR_LEFT : DIR_RIGHT)));
    Y.flash_t = 0;
}

static void input(void)
{
    if (voice_busy() || acting())
        Y.idle = 0;
    else if (++Y.idle == 15 * FPS && !Y.replayed) {
        Y.replayed = true; /* distracted? the coach calls it again, once */
        start_listen(true);
        return;
    }
    if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
        game_question_replay();
        Y.nq = 0;
        start_listen(true);
        return;
    }
    int dir, m = pressed_move(&dir);
    if (m < 0)
        return;
    Y.idle = 0;
    bool ok = m == Y.seq[Y.idx];
    BOT("ginnastica_do move=%c ok=%d\n", MOVES[m].code, ok);
    if (!ok && Y.guided) {
        sfx("blip"); /* together: only the blinking button counts */
        return;
    }
    flash_control(m, dir);
    if (Y.npressed < MAX_ACTS)
        Y.pressed[Y.npressed++] = m;
    queue_act(m, dir);
    if (!ok) {
        mistake(m);
        return;
    }
    /* the voice counts with her: "uno, due, tre..." */
    int p = Y.part_of[Y.idx], k = 0;
    for (int i = 0; i <= Y.idx; i++)
        k += Y.part_of[i] == p;
    if (m != MV_SALUTO && (Y.part[p].count > 1 || m == MV_SALTO)) {
        char id[8];
        snprintf(id, sizeof(id), "n%02d", clampi(k, 1, 20));
        say(id);
    }
    if (++Y.idx == Y.len) {
        Y.state = GI_FINISH;
        Y.t = 0;
    }
}

static void enter(void)
{
    memset(&Y, 0, sizeof(Y));
    Y.act = -1;
    Y.flash = -1;
    Y.x = HERO_X * 16;
    Y.last_sig = -1;
    hero_init(&G.hero, HERO_X, FLOOR);
    music_play(snd_find(SND_MUSIC, "palco"));
    if (!G.intro_done[GAME_GINNASTICA]) {
        /* the first times the coach shows what every button does, and Deva does
           it; later only "ascolta l'esercizio, poi fallo tu" (the legend stays) */
        G.intro_done[GAME_GINNASTICA] = true;
        Y.state = GI_INTRO;
        Y.intro_k = -1;
        say("gi_intro");
        bool buttons = G.prog.rounds[GAME_GINNASTICA] < 2;
        char id[32];
        for (int m = 0; m < MV_COUNT; m++) {
            Y.intro_clip[m] = -1;
            if (!buttons)
                continue;
            snprintf(id, sizeof(id), "gi_tasto_%s", MOVES[m].id);
            Y.intro_clip[m] = snd_find(SND_VOICE, id);
            say_then(id);
        }
    } else {
        new_routine();
        start_listen(true);
    }
}

static void update(void)
{
    Y.t++;
    Y.flash_t++;
    update_act();
    switch (Y.state) {
    case GI_INTRO: {
        sound_t cur = voice_current();
        for (int m = 0; m < MV_COUNT; m++)
            if (cur >= 0 && cur == Y.intro_clip[m] && Y.intro_k != m) {
                Y.intro_k = m;
                start_act(m, 1);
                flash_control(m, 1);
                Y.flash_t = -60; /* stays lit while she hears it */
            }
        walk_home();
        if ((btn_pressed(BTN_B) || btn_pressed(BTN_Y)) && Y.t > 20) { /* known already: skip */
            voice_stop();
            BOT("ginnastica intro skipped\n");
        }
        if (!voice_busy() && !acting() && Y.t > 30) {
            new_routine();
            start_listen(true);
        }
        break;
    }
    case GI_LISTEN: {
        walk_home();
        sound_t cur = voice_current();
        for (int p = 0; p < Y.nparts; p++)
            if (cur >= 0 && cur == Y.clip[p] && Y.shown < p + 1)
                Y.shown = p + 1;
        if (!voice_busy() && Y.t > 20 && Y.x == HERO_X * 16) {
            Y.shown = Y.nparts;
            say("balla_tocca"); /* "Tocca a te!" */
            start_input();
        }
        break;
    }
    case GI_INPUT:
        input();
        break;
    case GI_FINISH:
        if (!acting() && Y.t > 10)
            success();
        break;
    case GI_WRONG:
        if (Y.t > 30 && !voice_busy() && !acting()) {
            if (Y.x != HERO_X * 16)
                walk_home();
            else if (Y.guided)
                start_listen(false);
            else
                start_listen(true);
        }
        break;
    case GI_RIGHT:
        if (Y.t > 100 && !voice_busy()) {
            if (++Y.question >= G.cfg.questions_per_round) {
                game_round_done(GAME_GINNASTICA);
                return;
            }
            new_routine();
            start_listen(true);
        }
        break;
    }
}

/* ------------------------------------------------------------------ drawing */

static int card_x(int p) { return 160 - (Y.nparts * CARD_STEP - (CARD_STEP - CARD_W)) / 2 + p * CARD_STEP; }

static int done_in_part(int p)
{
    int k = 0;
    for (int i = 0; i < Y.idx; i++)
        k += Y.part_of[i] == p;
    return k;
}

/* the little Deva of a card: the pose of the move, turned for the rolls */
static void draw_pose_icon(int move, int cx, int bottom, int bob)
{
    switch (move) {
    case MV_SALTO: { /* up in the air: lifted, speed lines and her shadow far below */
        const sprite_t *sh = gfx_sprite("shadow");
        gfx_blit(sh, cx - sh->w / 2, bottom - 5, 0);
        hero_t h = posed(POSE_JUMP, false);
        h.x = cx;
        h.y = bottom - 16 + bob;
        h.no_shadow = true;
        hero_draw(&h, &G.prog);
        const sprite_t *s = gfx_sprite("gm_scia");
        gfx_blit(s, cx - s->w / 2, bottom - 14 + bob, 0);
        break;
    }
    case MV_CAPRIOLA: {
        const sprite_t *g = gfx_sprite("gm_giro");
        int cy = bottom - 30 + bob;
        draw_turned(posed(POSE_GIU, false), ROLL_PIVOT, cx, cy, 90);
        gfx_blit(g, cx - g->w / 2, cy - g->h / 2, 0);
        break;
    }
    case MV_RUOTA: {
        const sprite_t *a = gfx_sprite("gm_arco");
        int cy = bottom - 38 + bob;
        draw_turned(posed(POSE_JUMP, false), 30, cx, cy, 180);
        gfx_blit(a, cx - a->w / 2, cy - 36, 0);
        break;
    }
    default: { /* the salute: standing tall, arms up, a star over her head */
        hero_t h = posed(POSE_UP, false);
        h.x = cx;
        h.y = bottom - 3 + bob;
        h.no_shadow = true;
        hero_draw(&h, &G.prog);
        const sprite_t *st = gfx_sprite("star_on");
        gfx_blit(st, cx - st->w / 2, bottom - 74 + bob, 0);
        const sprite_t *sp = gfx_sprite("sparkle");
        if ((G.frame / 12) & 1) {
            gfx_blit(sp, cx - 19, bottom - 70, 0);
            gfx_blit(sp, cx + 14, bottom - 70, 0);
        }
        break;
    }
    }
}

static void draw_board(void)
{
    int cur = Y.state == GI_INPUT && Y.idx < Y.len ? Y.part_of[Y.idx] : -1;
    for (int p = 0; p < Y.nparts; p++) {
        int x = card_x(p), y = BOARD_Y;
        bool announced = Y.state == GI_INPUT || Y.state == GI_FINISH || Y.state == GI_WRONG ||
                         Y.state == GI_RIGHT || p < Y.shown;
        bool done = done_in_part(p) >= Y.part[p].count && Y.state != GI_LISTEN;
        bool sel = p == cur || (Y.state == GI_LISTEN && p == Y.shown - 1);
        int bob = sel && Y.state == GI_INPUT ? -swing((int)G.frame, 48, 1) : 0;
        if (Y.state == GI_RIGHT && Y.t < 90)
            bob = ((Y.t / 4 + Y.nparts - p) % 8) < 2 ? -3 : 0;
        ui_card(x, y + bob, CARD_W, CARD_H, sel);
        if (!announced)
            continue;
        if (Y.hidden && !Y.guided && Y.state == GI_INPUT && !done) { /* from memory: face down until done */
            gfx_text_big("?", x + CARD_W / 2, y + CARD_H / 2 + bob, NUM_SELECTED, 2);
            continue;
        }
        gfx_set_clip(x + 2, y + 2 + bob, x + CARD_W - 2, y + CARD_H - 2 + bob);
        draw_pose_icon(Y.part[p].move, x + CARD_W / 2, y + CARD_H - 2 + bob, 0);
        gfx_reset_state();
        if (Y.part[p].move != MV_SALUTO)
            gfx_number_big(Y.part[p].count, x + CARD_W - 9, y + 13 + bob, sel ? NUM_SELECTED : NUM_NORMAL);
        if (done)
            gfx_blit(gfx_sprite("mark_ok"), x - 4, y - 4 + bob, 0);
        if (Y.dots && Y.part[p].move == MV_SALTO) { /* level 1: a star for every jump */
            int n = Y.part[p].count, d = done_in_part(p);
            for (int i = 0; i < n; i++)
                gfx_blit(gfx_sprite(i < d ? "lstar_on" : "lstar_off"), x + CARD_W / 2 - n * 6 + i * 12, y + CARD_H + 3, 0);
        }
    }
}

static void draw_legend(void)
{
    /* the cross and the red button, on the wall: the one just used lights up;
       together, the next one blinks */
    int lit = -1;
    if (Y.flash >= 0 && Y.flash_t < 24)
        lit = Y.flash;
    else if (Y.state == GI_INPUT && Y.guided && ((Y.t / 12) & 1) && Y.idx < Y.len) {
        int m = Y.seq[Y.idx];
        lit = m == MV_SALTO ? 4 : (m == MV_CAPRIOLA ? DIR_DOWN : (m == MV_SALUTO ? DIR_UP : DIR_RIGHT));
    }
    hud_dpad(DPAD_X, DPAD_Y, lit >= 0 && lit < 4 ? lit : DIR_NONE);
    const sprite_t *a = gfx_sprite("btn_a");
    int k = lit == 4 ? 3 : 2;
    gfx_blit_scaled(a, DPAD_X + 20 - a->w * k / 2, DPAD_Y + 46, k, 0);
}

static void draw(void)
{
    amb_bg("bg_palestra");
    game_draw_stars(160, Y.stars, G.cfg.questions_per_round);
    if (Y.state != GI_INTRO)
        draw_board();
    draw_legend();
    draw_deva();
    fx_draw();
}

const scene_t SCENE_GINNASTICA = {enter, update, draw};

/* ------------------------------------------------------------------ QUIZ_SALTI (duels) */

static struct {
    int n, opt[3];
    int jump_t;   /* frames of the jump in progress, -1 = none */
    int jumps;    /* jumps done in this show */
    bool shown;   /* the show is over */
    int count_to; /* the guided count shows this many jumps */
    bool replay;  /* B: the jumps once more, then "quanti salti?" */
} QS;

#define JUMP_FRAMES 34

static const int SALTI_RANGE[5][2] = {{1, 3}, {2, 5}, {3, 6}, {4, 8}, {5, 10}};

static void salti_setup(quiz_t *q, int level)
{
    int lo = SALTI_RANGE[clampi(level, 1, 5) - 1][0], hi = SALTI_RANGE[clampi(level, 1, 5) - 1][1];
    QS.n = rng_range(lo, hi);
    int cand[5], nc = 0, others[2];
    for (int d = -2; d <= 3; d++)
        if (d && QS.n + d >= 1)
            cand[nc++] = QS.n + d;
    for (int i = nc - 1; i > 0; i--) {
        int j = rng_range(0, i), t = cand[i];
        cand[i] = cand[j];
        cand[j] = t;
    }
    others[0] = cand[0];
    others[1] = cand[1];
    q->card = QUIZ_CARD_NUMBER;
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        QS.opt[i] = i == q->correct ? QS.n : others[k++];
    QS.jump_t = -1;
    QS.jumps = 0;
    QS.shown = false;
    QS.count_to = 0;
    QS.replay = false;
}

static bool salti_prepare(quiz_t *q)
{
    if (q->t == 1)
        say("gi_guarda_salti");
    if (q->t < 20 || voice_busy())
        return false;
    if (QS.jump_t < 0) {
        if (QS.jumps >= QS.n)
            return q->t > 20;
        QS.jump_t = 0;
        sfx("boing");
    }
    return false;
}

static void salti_tick(quiz_t *q)
{
    if (QS.jump_t >= 0 && ++QS.jump_t >= JUMP_FRAMES) {
        QS.jump_t = -1;
        QS.jumps++;
    }
    if (QS.replay) {
        if (q->state != Q_INPUT) { /* she answered (or the help began): the show stops */
            QS.replay = false;
        } else if (QS.jump_t < 0 && !voice_busy()) {
            if (QS.jumps < QS.n) {
                QS.jump_t = 0;
                sfx("boing");
            } else {
                QS.replay = false;
                say("gi_quanti");
            }
        }
    }
}

static void salti_ask(quiz_t *q)
{
    if (q->state == Q_INPUT) { /* B (or a long silence): the jumps again, straight away */
        QS.jumps = 0;
        QS.jump_t = -1;
        QS.replay = true;
        voice_stop();
        BOT("salti replay n=%d\n", QS.n);
        return;
    }
    say("gi_quanti");
}

static void salti_hover(quiz_t *q, int i)
{
    char id[8];
    snprintf(id, sizeof(id), "n%02d", clampi(QS.opt[i], 1, 20));
    say(id);
}

static bool salti_hint(quiz_t *q)
{
    /* count them together: the jumps again, one number each */
    if (q->hint_step < QS.n) {
        char id[8];
        snprintf(id, sizeof(id), "n%02d", clampi(q->hint_step + 1, 1, 20));
        QS.jump_t = 0;
        sfx("boing");
        say(id);
        q->hint_wait = JUMP_FRAMES;
        return false;
    }
    say("gi_quanti");
    return true;
}

static void salti_draw(quiz_t *q)
{
    int cx = q->panel_cx, base = q->panel_cy + 52;
    const sprite_t *tr = gfx_sprite("trampolino");
    gfx_blit(tr, cx - tr->w / 2, base - 8, 0);
    const char *name = q->duel ? story_foe_sprite(clampi(story_chapter(), 0, CH_COUNT - 1), 'c') : "mo_ciuffone_b";
    const sprite_t *s = gfx_sprite(name);
    int lift = 0, sdy = 0, sdx = 0;
    if (QS.jump_t >= 0) { /* 0.13: the trampoline throws it up long and thin, it lands squashed */
        int k = QS.jump_t, n = JUMP_FRAMES;
        lift = 44 * 4 * k * (n - k) / (n * n);
        int sq = s->h / 8, v = absi(n - 2 * k) * sq / imax(1, n); /* fast near the trampoline */
        sdy = (k < 2 || k > n - 2) ? -sq : v;
        sdx = (k < 2 || k > n - 2) ? sq : -v / 2;
    }
    actor_draw(name, cx, base - lift, 1, 0, 0, sdy, sdx);
}

static void salti_card(quiz_t *q, int i, int cx, int cy, int style)
{
    int ns = style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL);
    gfx_number_big(QS.opt[i], cx, cy, ns);
}

static void salti_value(quiz_t *q, int i, char *buf, size_t n) { snprintf(buf, n, "%d", QS.opt[i]); }

static const char *salti_subject(quiz_t *q)
{
    static char s[24];
    snprintf(s, sizeof(s), "salti:%d", QS.n);
    return s;
}

const quiz_def_t QUIZ_SALTI = {
    .name = "ginnastica",
    .game = GAME_GINNASTICA,
    .bg = "bg_palestra",
    .card = QUIZ_CARD_NUMBER,
    .setup = salti_setup,
    .prepare = salti_prepare,
    .tick = salti_tick,
    .ask = salti_ask,
    .hover = salti_hover,
    .hint = salti_hint,
    .draw = salti_draw,
    .card_draw = salti_card,
    .card_value = salti_value,
    .subject = salti_subject,
};
