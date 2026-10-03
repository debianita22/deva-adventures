/* Deva's Awesome Adventures - "Il sentiero" (0.10.0): a first taste of coding.
 *
 * A garden of stepping stones seen from above: little Deva (a pawn of a board
 * game, drawn small so that nothing hides behind her) on one stone, a star on
 * another. She plans the way with the arrows of the cross - each press puts
 * an arrow in the row at the bottom (the program), the yellow button takes
 * the last one away - then the red button: Deva walks the arrows one by one.
 * A rock or the edge of the garden stops her (the arrow that bumped blinks,
 * so the mistake can be found and fixed); arriving elsewhere is "quasi!".
 *
 * Levels: 1 the star straight ahead, 1-3 stones (dotted footprints show
 * where the arrows lead); 2 the star round a corner, footprints; 3 a rock or
 * two in the way, no footprints; 4 first pick the flower, then the star;
 * 5 longer ways among more rocks. After two mistakes the program of the
 * shortest way shows as ghost arrows: she presses them one by one, then red.
 *
 * QUIZ_STRADA, the duel question of this game (the third adventure): a small
 * garden in the panel, Deva and the magic note; which row of arrows takes her
 * there?
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
#include "quiz.h"
#include "story.h"
#include "ui.h"

#define COLS 6
#define ROWS 4
#define TW 40
#define TH 32
#define GX 40          /* top left of the garden of stones */
#define GY 40
#define MAX_PROG 8
#define STEP_T 20      /* frames of one step */
#define SLOT_STEP 32   /* the program row */
#define PROG_Y 196
#define PROG_X (160 - (MAX_PROG * SLOT_STEP - 2) / 2)

enum { D_SU, D_GIU, D_SX, D_DX };
static const int DX[4] = {0, 0, -1, 1}, DY[4] = {-1, 1, 0, 0};
static const char *const ARROW[4] = {"arrow_up", "arrow_down", "arrow_left", "arrow_right"};
static const char *const SARROW[4] = {"sarrow_up", "sarrow_down", "sarrow_left", "sarrow_right"};
static const char *const TONE[4] = {"tone_su", "tone_giu", "tone_sx", "tone_dx"};
static const char DCH[4] = {'U', 'D', 'L', 'R'};
static const int BTN_OF[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};

/* ------------------------------------------------------------------ the garden */

typedef struct {
    int cols, rows;
    bool rock[ROWS][COLS];
    int sx, sy, gx, gy, fx, fy; /* start, star (goal), flower (-1: none) */
} garden_t;

static bool inside(const garden_t *g, int x, int y) { return x >= 0 && y >= 0 && x < g->cols && y < g->rows; }
static bool free_at(const garden_t *g, int x, int y) { return inside(g, x, y) && !g->rock[y][x]; }

/* the shortest way from (x0, y0) to (x1, y1) as arrows (-1 = none), its length */
static int shortest(const garden_t *g, int x0, int y0, int x1, int y1, int *out, int max)
{
    int dist[ROWS][COLS], from[ROWS][COLS], qx[ROWS * COLS], qy[ROWS * COLS], head = 0, tail = 0;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            dist[y][x] = -1;
    dist[y0][x0] = 0;
    qx[tail] = x0;
    qy[tail++] = y0;
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        if (x == x1 && y == y1)
            break;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (free_at(g, nx, ny) && dist[ny][nx] < 0) {
                dist[ny][nx] = dist[y][x] + 1;
                from[ny][nx] = d;
                qx[tail] = nx;
                qy[tail++] = ny;
            }
        }
    }
    int n = dist[y1][x1];
    if (n < 0 || n > max)
        return -1;
    for (int k = n - 1, x = x1, y = y1; k >= 0; k--) {
        int d = from[y][x];
        out[k] = d;
        x -= DX[d];
        y -= DY[d];
    }
    return n;
}

/* walks a program: where it stops, the arrow that bumped (-1), the flower picked */
typedef struct {
    int x, y, bumped, steps;
    bool flower, arrived;
} walk_t;

static walk_t walk(const garden_t *g, const int *prog, int n)
{
    walk_t w = {g->sx, g->sy, -1, 0, false, false};
    for (int i = 0; i < n; i++) {
        int nx = w.x + DX[prog[i]], ny = w.y + DY[prog[i]];
        if (!free_at(g, nx, ny)) {
            w.bumped = i;
            return w;
        }
        w.x = nx;
        w.y = ny;
        w.steps = i + 1;
        if (nx == g->fx && ny == g->fy)
            w.flower = true;
        if (nx == g->gx && ny == g->gy && (g->fx < 0 || w.flower)) {
            w.arrived = true;
            return w;
        }
    }
    return w;
}

static void prog_string(const int *p, int n, char *buf, size_t size)
{
    size_t k = 0;
    for (int i = 0; i < n && k + 1 < size; i++)
        buf[k++] = DCH[p[i]];
    buf[k] = 0;
}

static int solve(const garden_t *g, int *sol)
{
    if (g->fx < 0)
        return shortest(g, g->sx, g->sy, g->gx, g->gy, sol, MAX_PROG);
    int a = shortest(g, g->sx, g->sy, g->fx, g->fy, sol, MAX_PROG);
    if (a < 0)
        return -1;
    garden_t g2 = *g; /* from the flower on */
    g2.sx = g->fx;
    g2.sy = g->fy;
    int b = shortest(&g2, g->fx, g->fy, g->gx, g->gy, sol + a, MAX_PROG - a);
    return b < 0 ? -1 : a + b;
}

/* is one of the two L-shaped routes (first across then down, or the other way) blocked by a rock? */
static bool route_blocked(const garden_t *g)
{
    int sx = g->gx > g->sx ? 1 : -1, sy = g->gy > g->sy ? 1 : -1;
    for (int first_x = 0; first_x < 2; first_x++) {
        int x = g->sx, y = g->sy;
        for (int leg = 0; leg < 2; leg++) {
            bool horiz = (leg == 0) == (first_x == 1);
            int cnt = horiz ? absi(g->gx - g->sx) : absi(g->gy - g->sy);
            for (int i = 0; i < cnt; i++) {
                if (horiz)
                    x += sx;
                else
                    y += sy;
                if (g->rock[y][x])
                    return true;
            }
        }
    }
    return false;
}

/* a new garden for the level: returns the length of its shortest program */
static int make_garden(garden_t *g, int level, int cols, int rows, int *sol)
{
    for (int tries = 0; tries < 4000; tries++) {
        memset(g, 0, sizeof(*g));
        g->cols = cols;
        g->rows = rows;
        g->fx = g->fy = -1;
        g->sx = rng_range(0, cols - 1);
        g->sy = rng_range(0, rows - 1);
        int dist, lo, hi, rocks = 0;
        if (level <= 1) { /* straight ahead */
            int d = rng_range(0, 3), len = rng_range(1, 3);
            g->gx = g->sx + DX[d] * len;
            g->gy = g->sy + DY[d] * len;
            lo = 1, hi = 3;
        } else {
            g->gx = rng_range(0, cols - 1);
            g->gy = rng_range(0, rows - 1);
            if (level == 2 && (g->gx == g->sx || g->gy == g->sy))
                continue; /* round a corner */
            lo = level == 2 ? 2 : 3;
            hi = level == 2 ? 4 : (level == 3 ? 6 : (level == 4 ? 7 : 8));
            rocks = level == 3 ? rng_range(1, 2) : (level == 4 ? rng_range(0, 1) : rng_range(2, 3));
        }
        if (!inside(g, g->gx, g->gy) || (g->gx == g->sx && g->gy == g->sy))
            continue;
        dist = absi(g->gx - g->sx) + absi(g->gy - g->sy);
        if (dist < lo || dist > hi)
            continue;
        for (int r = 0; r < rocks; r++) { /* rocks on the way, mostly */
            int x = rng_range(imin(g->sx, g->gx), imax(g->sx, g->gx)), y = rng_range(imin(g->sy, g->gy), imax(g->sy, g->gy));
            if ((x == g->sx && y == g->sy) || (x == g->gx && y == g->gy))
                x = rng_range(0, cols - 1), y = rng_range(0, rows - 1);
            if ((x == g->sx && y == g->sy) || (x == g->gx && y == g->gy))
                continue;
            g->rock[y][x] = true;
        }
        if (level == 4) { /* the flower first */
            g->fx = rng_range(0, cols - 1);
            g->fy = rng_range(0, rows - 1);
            if (g->rock[g->fy][g->fx] || (g->fx == g->sx && g->fy == g->sy) || (g->fx == g->gx && g->fy == g->gy))
                continue;
        }
        int n = solve(g, sol);
        if (n < lo || n > hi)
            continue;
        if (level == 3 && rocks > 0 && n == dist && tries < 3000 && !route_blocked(g))
            continue; /* the rocks must stand in the way of a straight route */
        return n;
    }
    /* a plain one, never reached in practice */
    memset(g, 0, sizeof(*g));
    g->cols = cols, g->rows = rows, g->fx = g->fy = -1;
    g->sx = 0, g->sy = 0, g->gx = 2, g->gy = 0;
    return solve(g, sol);
}

/* ------------------------------------------------------------------ the scene */

enum { S_INTRO, S_INPUT, S_RUN, S_WRONG, S_BACK, S_RIGHT };
enum { PW_STAND, PW_STEP, PW_HOORAY, PW_OH }; /* the looks of the little Deva */

static struct {
    int state, t, level;
    garden_t g;
    int sol[MAX_PROG], nsol;
    int prog[MAX_PROG], nprog;
    walk_t w;          /* the result of the run */
    int run_i;         /* step being walked */
    int x, y;          /* Deva's stone */
    bool picked;       /* the flower */
    bool guided;
    int attempts, question, questions, stars;
    int idle;
    bool reminded;
    int pop_t;         /* the last arrow put in the row pops */
    bool face_right;   /* up and down keep the way she faced */
    bool clear_prog;   /* back to the start: the row empties (now it is done together) */
    int px, py;        /* the little Deva: her feet */
    int look, look_t;  /* PW_*, frames left (then she stands) */
} S;

static int stone_cx(int x) { return GX + x * TW + TW / 2; }
static int stone_cy(int y) { return GY + y * TH + TH / 2; }
static int feet_y(int y) { return GY + y * TH + TH - 5; }

static void log_attempt(bool ok)
{
    char target[16], chosen[16], subject[48];
    prog_string(S.sol, S.nsol, target, sizeof(target));
    prog_string(S.prog, S.nprog, chosen, sizeof(chosen));
    int rocks = 0;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            rocks += S.g.rock[y][x];
    snprintf(subject, sizeof(subject), "passi:%d:sassi:%d%s%s", S.nsol, rocks, S.g.fx >= 0 ? ":fiore" : "",
             S.guided ? ":insieme" : "");
    game_log_answer("sentiero", S.level, subject, target, chosen, ok, S.attempts + 1);
    BOT("answer pick=%d ok=%d attempt=%d\n", S.nprog, ok, S.attempts + 1);
}

static void pawn_look(int look, int frames)
{
    S.look = look;
    S.look_t = frames;
}

static void pawn_at_stone(void)
{
    S.px = stone_cx(S.x);
    S.py = feet_y(S.y);
}

static void start_input(void)
{
    S.state = S_INPUT;
    S.t = 0;
    S.idle = 0;
    S.x = S.g.sx;
    S.y = S.g.sy;
    S.picked = false;
    pawn_at_stone();
    game_question_ready();
    char sol[16];
    prog_string(S.sol, S.nsol, sol, sizeof(sol));
    BOT("sentiero ready level=%d sol=%s prog=%d guided=%d flower=%d\n", S.level, sol, S.nprog, S.guided,
        S.g.fx >= 0);
}

static void ask(void)
{
    if (S.guided) {
        say("sen_insieme"); /* "Facciamolo insieme: premi le frecce che brillano, poi il rosso!" */
        return;
    }
    say(S.g.fx >= 0 ? "sen_fiore" : "sen_stella");
    if (S.question == 0 && S.level <= 2)
        say_then("sen_frecce");
}

static void new_garden(void)
{
    S.level = clampi(G.prog.level[GAME_SENTIERO], 1, 5);
    S.nsol = make_garden(&S.g, S.level, COLS, ROWS, S.sol);
    S.nprog = 0;
    S.attempts = 0;
    S.guided = false;
    S.reminded = false;
    char sol[16];
    prog_string(S.sol, S.nsol, sol, sizeof(sol));
    BOT("sentiero garden level=%d start=%d,%d goal=%d,%d flower=%d,%d sol=%s\n", S.level, S.g.sx, S.g.sy, S.g.gx,
        S.g.gy, S.g.fx, S.g.fy, sol);
    sfx("pop");
    start_input();
    ask();
}

static void enter(void)
{
    memset(&S, 0, sizeof(S));
    S.questions = clampi(G.cfg.questions_per_round, 1, 3); /* a garden takes longer than a card */
    music_play(snd_find(SND_MUSIC, "palco"));
    S.face_right = true;
    if (!G.intro_done[GAME_SENTIERO]) {
        G.intro_done[GAME_SENTIERO] = true;
        S.state = S_INTRO;
        say("sen_intro");
        S.level = clampi(G.prog.level[GAME_SENTIERO], 1, 5);
        S.nsol = make_garden(&S.g, S.level, COLS, ROWS, S.sol);
        S.x = S.g.sx;
        S.y = S.g.sy;
        pawn_at_stone();
        pawn_look(PW_HOORAY, 110); /* hello! */
    } else {
        new_garden();
    }
}

static void run(void)
{
    S.w = walk(&S.g, S.prog, S.nprog);
    S.state = S_RUN;
    S.t = 0;
    S.run_i = 0;
    S.picked = false;
    voice_stop();
    char p[16];
    prog_string(S.prog, S.nprog, p, sizeof(p));
    BOT("sentiero run prog=%s arrived=%d bumped=%d\n", p, S.w.arrived, S.w.bumped);
}

static void success(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
    bool first_try = S.attempts == 0 && !S.guided;
    log_attempt(true);
    sfx("fanfare");
    say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
    if (G.cfg.rumble)
        frontend_rumble(1, 10);
    pawn_look(PW_HOORAY, 100);
    fx_confetti(S.px, S.py - 40, 30);
    fx_burst(stone_cx(S.g.gx), stone_cy(S.g.gy), 14);
    fx_hearts(S.px + 4, S.py - 34, 3);
    S.stars++;
    G.prog.stars_total++;
    game_star_fly(stone_cx(S.g.gx), stone_cy(S.g.gy), 160, S.stars - 1, S.questions);
    if (game_level_result(GAME_SENTIERO, first_try, S.guided)) {
        say_then("livello_su");
        fx_starfall(26);
    }
    S.state = S_RIGHT;
    S.t = 0;
    input_block(30);
}

static void mistake(void)
{
    log_attempt(false);
    S.attempts++;
    sfx("boop");
    pawn_look(PW_OH, 45);
    if (S.attempts >= 2 && !S.guided) {
        S.guided = true; /* back at the start the row empties and the right arrows shine ("facciamolo insieme") */
        S.clear_prog = true;
        voice_stop();
        BOT("hint\n");
    } else if (S.w.bumped >= 0) { /* a rock, or the edge of the garden */
        int nx = S.w.x + DX[S.prog[S.w.bumped]], ny = S.w.y + DY[S.prog[S.w.bumped]];
        say(inside(&S.g, nx, ny) ? "sen_sasso" : "sen_fuori");
        say_then("sen_giallo"); /* "Il bottone giallo toglie l'ultima freccia." */
    } else if (S.g.fx >= 0 && !S.w.flower) {
        say("sen_senza_fiore");
    } else {
        say("sen_quasi");
    }
    S.state = S_WRONG;
    S.t = 0;
    input_block(20);
}

static void add_arrow(int d)
{
    if (S.nprog >= MAX_PROG) {
        sfx("boop");
        return;
    }
    if (S.guided && (S.nprog >= S.nsol || d != S.sol[S.nprog])) {
        sfx("blip"); /* together: only the arrow that shines */
        return;
    }
    S.prog[S.nprog++] = d;
    S.pop_t = 0;
    sfx(TONE[d]);
    fx_twinkle(PROG_X + (S.nprog - 1) * SLOT_STEP + 15, PROG_Y + 2, 0);
    BOT("sentiero add=%c n=%d\n", DCH[d], S.nprog);
}

static void update_input(void)
{
    if (voice_busy())
        S.idle = 0;
    else if (++S.idle == 15 * FPS && !S.reminded) {
        S.reminded = true;
        ask();
    }
    for (int d = 0; d < 4; d++)
        if (btn_pressed(BTN_OF[d])) {
            S.idle = 0;
            add_arrow(d);
            return;
        }
    if (btn_pressed(BTN_B)) {
        if (S.nprog > 0 && !S.guided) {
            S.nprog--;
            sfx("pop");
            BOT("sentiero del n=%d\n", S.nprog);
        } else {
            sfx("boop");
        }
    } else if (btn_pressed(BTN_Y)) {
        game_question_replay();
        ask();
    } else if (btn_pressed(BTN_A)) {
        if (S.nprog == 0 || (S.guided && S.nprog < S.nsol)) {
            sfx("boop");
            say(S.nprog == 0 ? "sen_frecce" : "sen_insieme");
        } else {
            sfx("star");
            run();
        }
    }
}

static void update_run(void)
{
    int steps = S.w.bumped >= 0 ? S.w.bumped + 1 : S.w.steps; /* the bump is a step that comes back */
    int i = S.t / STEP_T, k = S.t % STEP_T;
    if (k == 0 && i < steps) { /* a new step */
        S.run_i = i;
        int d = S.prog[i];
        sfx(TONE[d]);
        if (d == D_DX || d == D_SX)
            S.face_right = d == D_DX;
    }
    if (i < steps) {
        int d = S.prog[i];
        bool bump = i == S.w.bumped;
        int u = k * 256 / STEP_T, e = bump ? (u < 128 ? u : 256 - u) / 2 : u; /* a bump goes half way and back */
        int x0 = stone_cx(S.x), y0 = feet_y(S.y);
        S.px = x0 + DX[d] * TW * e / 256;
        S.py = y0 + DY[d] * TH * e / 256;
        pawn_look(bump && k >= STEP_T / 2 ? PW_OH : ((k / 5) & 1 ? PW_STEP : PW_STAND), 2);
        if (bump && k == STEP_T / 2) {
            sfx("boing");
            fx_puff(S.px + DX[d] * 14, S.py - 14);
        }
        if (k == STEP_T - 1 && !bump) { /* arrived on the next stone */
            S.x += DX[d];
            S.y += DY[d];
            pawn_at_stone();
            fx_twinkle(S.px + (i & 1 ? 5 : -5), S.py - 1, 0);
            if (S.x == S.g.fx && S.y == S.g.fy && !S.picked) {
                S.picked = true;
                sfx("sparkle");
                fx_sparkles(stone_cx(S.x), stone_cy(S.y) - 6, 12, 10);
            }
        }
        return;
    }
    if (S.t >= steps * STEP_T + 12) {
        if (S.w.arrived)
            success();
        else
            mistake();
    }
}

static void update(void)
{
    S.t++;
    S.pop_t++;
    if (S.look_t > 0 && --S.look_t == 0)
        S.look = PW_STAND;
    switch (S.state) {
    case S_INTRO:
        if (S.t > 30 && !voice_busy())
            new_garden();
        break;
    case S_INPUT:
        update_input();
        break;
    case S_RUN:
        update_run();
        break;
    case S_WRONG:
        if (S.t > 40 && !voice_busy()) { /* back to the start, with a little puff */
            fx_puff(S.px, S.py - 16);
            S.state = S_BACK;
            S.t = 0;
        }
        break;
    case S_BACK:
        if (S.t == 8) {
            if (S.clear_prog) {
                S.nprog = 0;
                S.clear_prog = false;
            }
            start_input();
            fx_sparkles(S.px, S.py - 14, 12, 8);
            if (S.guided)
                ask();
        }
        break;
    case S_RIGHT:
        if (S.t > 100 && !voice_busy()) {
            if (++S.question >= S.questions)
                game_round_done(GAME_SENTIERO);
            else
                new_garden();
        }
        break;
    }
}

/* ------------------------------------------------------------------ drawing */

static void draw_garden(const garden_t *g, int gx0, int gy0, int tw, int th, bool small)
{
    for (int y = 0; y < g->rows; y++)
        for (int x = 0; x < g->cols; x++) {
            int px = gx0 + x * tw, py = gy0 + y * th;
            bool alt = ((x + y) & 1) != 0;
            gfx_round_rect(px + 2, py + 3, tw - 4, th - 4, rgb565(0x5d, 0x8a, 0x4a), rgb565(0x5d, 0x8a, 0x4a));
            gfx_round_rect(px + 2, py + 1, tw - 4, th - 4, alt ? rgb565(0xf3, 0xe6, 0xd0) : rgb565(0xea, 0xdb, 0xc0),
                           rgb565(0x9c, 0x86, 0x6a));
            if (!small && ((x * 7 + y * 3) % 5) == 0) /* a little pebble on some stones */
                gfx_fill_rect(px + 8 + (x * 5 % 17), py + 6 + (y * 3 % 11), 2, 2, rgb565(0xc9, 0xb8, 0x9a));
            if (g->rock[y][x]) {
                const sprite_t *r = gfx_sprite(small ? "sasso_s" : "sasso");
                gfx_blit(r, px + (tw - r->w) / 2, py + th - r->h - (small ? 2 : 3), 0);
            }
        }
}

static void draw_program(void)
{
    for (int i = 0; i < MAX_PROG; i++) {
        int x = PROG_X + i * SLOT_STEP, y = PROG_Y;
        bool cur = (S.state == S_INPUT && i == S.nprog) || (S.state == S_RUN && i == S.run_i);
        bool bumped = (S.state == S_WRONG || S.state == S_BACK || S.state == S_INPUT) && S.w.bumped == i &&
                      i < S.nprog && S.attempts > 0 && !S.guided;
        if (cur && (S.state == S_RUN || ((G.frame / 16) & 1)))
            gfx_blit(gfx_sprite("scard_sel"), x - 2, y - 2, 0);
        else
            gfx_blit(gfx_sprite(i < S.nprog ? "scard" : "scard_off"), x, y, 0);
        if (i < S.nprog) {
            const sprite_t *a = gfx_sprite(ARROW[S.prog[i]]);
            int dy = (i == S.nprog - 1 && S.pop_t < 6) ? -(6 - S.pop_t) / 2 : 0;
            if (bumped && ((G.frame / 8) & 1)) /* the arrow that bumped blinks red */
                gfx_set_tint(rgb565(0xe5, 0x39, 0x35), 150);
            gfx_blit(a, x + 3, y + 3 + dy, 0);
            gfx_set_tint(0, 0);
        } else if (S.guided && i < S.nsol && S.state == S_INPUT) { /* the ghost of the arrow to press */
            const sprite_t *a = gfx_sprite(ARROW[S.sol[i]]);
            gfx_set_tint(rgb565(0xff, 0xf4, 0xfa), i == S.nprog && ((G.frame / 10) & 1) ? 60 : 170);
            gfx_blit(a, x + 3, y + 3, 0);
            gfx_set_tint(0, 0);
        }
    }
}

/* the little Deva: her shadow on the stone, a hop when she is happy */
static void draw_pawn(void)
{
    static const char *const LOOK[4] = {"deva_mini", "deva_mini_passo", "deva_mini_evviva", "deva_mini_oh"};
    const sprite_t *p = gfx_sprite(LOOK[clampi(S.look, 0, 3)]);
    int hop = 0;
    if (S.look == PW_HOORAY) {
        int ph = G.frame % 16;
        hop = -(ph * (16 - ph)) / 12;
    }
    gfx_fill_rect(S.px - 6, S.py, 12, 2, rgb565(0x9c, 0x86, 0x6a));
    gfx_fill_rect(S.px - 4, S.py + 2, 8, 1, rgb565(0x9c, 0x86, 0x6a));
    gfx_blit(p, S.px - p->w / 2, S.py - p->h + 1 + hop, S.face_right ? 0 : GFX_FLIP_H);
    if (S.picked && (S.state == S_RUN || S.state == S_RIGHT || S.state == S_WRONG)) { /* the flower in her hand */
        const sprite_t *f = gfx_sprite("fiorellino_s");
        int fx_ = S.face_right ? S.px + 7 : S.px - 7 - f->w;
        gfx_blit(f, fx_, S.py - 14 - f->h / 2 + hop, 0);
    }
}

/* levels 1-2: where the arrows lead, as little footprints on the stones */
static void draw_preview(void)
{
    if (S.level > 2 || S.guided || S.state != S_INPUT)
        return;
    int x = S.g.sx, y = S.g.sy;
    for (int i = 0; i < S.nprog; i++) {
        int nx = x + DX[S.prog[i]], ny = y + DY[S.prog[i]];
        if (!free_at(&S.g, nx, ny))
            break;
        x = nx;
        y = ny;
        int cx = stone_cx(x), cy = stone_cy(y);
        gfx_fill_rect(cx - 5, cy + 2, 3, 4, rgb565(0xc4, 0x9a, 0x6c));
        gfx_fill_rect(cx + 2, cy - 1, 3, 4, rgb565(0xc4, 0x9a, 0x6c));
    }
}

static void draw(void)
{
    amb_bg("bg_prato");
    game_draw_stars(160, S.stars, S.questions);
    draw_garden(&S.g, GX, GY, TW, TH, false);
    draw_preview();
    if (S.g.fx >= 0 && !(S.picked && S.state != S_INPUT)) {
        const sprite_t *f = gfx_sprite("fiorellino");
        gfx_blit(f, stone_cx(S.g.fx) - f->w / 2, stone_cy(S.g.fy) - f->h / 2 - 2, 0);
    }
    if (S.state != S_RIGHT || S.t < 8) { /* the star waiting on its stone */
        const sprite_t *st = gfx_sprite("star_on");
        int bob = -swing((int)G.frame, 54, 1);
        gfx_blit_scaled(st, stone_cx(S.g.gx) - st->w, stone_cy(S.g.gy) - st->h - 1 + bob, 2, 0);
    }
    draw_pawn();
    if (S.state != S_INTRO)
        draw_program();
    if (S.state == S_INPUT && S.nprog > 0 && S.idle > 3 * FPS && ((S.t / 20) & 1) &&
        (!S.guided || S.nprog == S.nsol))
        gfx_blit(gfx_sprite("btn_a"), PROG_X + MAX_PROG * SLOT_STEP, PROG_Y + 8, 0);
    fx_draw();
}

const scene_t SCENE_SENTIERO = {enter, update, draw};

/* ------------------------------------------------------------------ QUIZ_STRADA (duel) */

#define QC 5 /* a small garden in the panel: 5 x 3 stones */
#define QR 3
#define QTW 30
#define QTH 26

static struct {
    garden_t g;
    int opt[3][4], n; /* three rows of arrows, the same length */
    int show_t;       /* guided help: mini Deva walking the right way, -1 = still */
} Q2;

static bool same_prog(const int *a, const int *b, int n)
{
    for (int i = 0; i < n; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

static void strada_setup(quiz_t *q, int level)
{
    garden_t *g = &Q2.g;
    int sol[MAX_PROG];
    for (int tries = 0;; tries++) {
        memset(g, 0, sizeof(*g));
        g->cols = QC, g->rows = QR, g->fx = g->fy = -1;
        g->sx = rng_range(0, 1);
        g->sy = rng_range(0, QR - 1);
        g->gx = rng_range(2, QC - 1);
        g->gy = rng_range(0, QR - 1);
        if (level >= 3 && rng_range(0, 1)) { /* a rock in the way */
            int ry, rx;
            rng_pair(0, QR - 1, 1, QC - 2, &ry, &rx);
            g->rock[ry][rx] = true;
        }
        if (g->rock[g->gy][g->gx] || g->rock[g->sy][g->sx])
            continue;
        Q2.n = solve(g, sol);
        int want = level <= 1 ? 2 : (level <= 3 ? 3 : 4);
        if (Q2.n >= 2 && Q2.n <= 4 && (Q2.n == want || tries > 300))
            break;
    }
    q->card = QUIZ_CARD_PICTURE;
    q->correct = rng_range(0, 2);
    for (int i = 0; i < Q2.n; i++)
        Q2.opt[q->correct][i] = sol[i];
    for (int k = 0, made = 0; made < 2 && k < 500; k++) { /* two wrong rows of the same length */
        int i = made < q->correct ? made : made + 1, p[4];
        if (made == 0 || k > 200) {
            for (int j = 0; j < Q2.n; j++)
                p[j] = rng_range(0, 3);
        } else { /* the right one with one arrow changed: the lookalike */
            memcpy(p, sol, sizeof(int) * (size_t)Q2.n);
            int at, d;
            rng_pair(0, Q2.n - 1, 0, 3, &at, &d);
            p[at] = d;
        }
        garden_t g2 = *g;
        g2.fx = g2.fy = -1;
        if (walk(&g2, p, Q2.n).arrived)
            continue; /* it must not arrive */
        bool dup = same_prog(p, sol, Q2.n);
        for (int j = 0; j < made && !dup; j++)
            dup = same_prog(p, Q2.opt[j < q->correct ? j : j + 1], Q2.n);
        if (dup)
            continue;
        memcpy(Q2.opt[i], p, sizeof(int) * (size_t)Q2.n);
        made++;
    }
    Q2.show_t = -1;
}

static bool strada_prepare(quiz_t *q) { return q->t > 16; }

static void strada_ask(quiz_t *q)
{
    /* "Quale strada porta Deva alla gemma (stella, nota, chiave) magica?": what waits at the end */
    static const char *const LINE[ARC_COUNT] = {"sen_duello_gemma", "sen_duello_stella", "sen_duello",
                                                "sen_duello_chiave"};
    const char *id = LINE[clampi(story_arc(), 0, ARC_COUNT - 1)];
    say(snd_exists(SND_VOICE, id) ? id : "sen_duello");
}

static void strada_hover(quiz_t *q, int i) { sfx(TONE[Q2.opt[i][0]]); }

static bool strada_hint(quiz_t *q)
{
    if (q->hint_step == 0) {
        say("sen_guarda"); /* "Guarda Deva: fa la strada giusta." */
        Q2.show_t = 0;
        return false;
    }
    if (Q2.show_t >= 0 && Q2.show_t < Q2.n * 30 + 20)
        return false;
    say("sen_scegli");
    return true;
}

static void strada_tick(quiz_t *q)
{
    if (Q2.show_t >= 0 && Q2.show_t < Q2.n * 30 + 20) {
        if (Q2.show_t % 30 == 0 && Q2.show_t / 30 < Q2.n)
            sfx(TONE[Q2.opt[q->correct][Q2.show_t / 30]]);
        Q2.show_t++;
    }
}

static void strada_draw(quiz_t *q)
{
    int gx0 = q->panel_cx - QC * QTW / 2, gy0 = q->panel_cy - QR * QTH / 2 + 4;
    draw_garden(&Q2.g, gx0, gy0, QTW, QTH, true);
    /* the note of the chapter waiting, and mini Deva */
    const char *gem = q->duel ? story_ch(clampi(story_chapter(), 0, CH_COUNT - 1))->gem : "star_on";
    const sprite_t *n = gfx_sprite(gem), *d = gfx_sprite("deva_mini");
    int bob = -swing((int)G.frame, 48, 1);
    gfx_blit(n, gx0 + Q2.g.gx * QTW + (QTW - n->w) / 2, gy0 + Q2.g.gy * QTH + (QTH - n->h) / 2 - 3 + bob, 0);
    int x = Q2.g.sx, y = Q2.g.sy, fx_ = 0, fy_ = 0;
    bool left = false;
    if (Q2.show_t >= 0) { /* the guided help: she walks the right way */
        int steps = imin(Q2.show_t / 30, Q2.n), k = Q2.show_t % 30;
        for (int i = 0; i < steps; i++) {
            x += DX[Q2.opt[q->correct][i]], y += DY[Q2.opt[q->correct][i]];
            left = Q2.opt[q->correct][i] == D_SX ? true : (Q2.opt[q->correct][i] == D_DX ? false : left);
        }
        if (steps < Q2.n) {
            int dd = Q2.opt[q->correct][steps];
            fx_ = DX[dd] * QTW * k / 30;
            fy_ = DY[dd] * QTH * k / 30;
            left = dd == D_SX ? true : (dd == D_DX ? false : left);
            if ((k / 6) & 1)
                d = gfx_sprite("deva_mini_passo");
        } else {
            d = gfx_sprite("deva_mini_evviva");
        }
    }
    gfx_blit(d, gx0 + x * QTW + (QTW - d->w) / 2 + fx_, gy0 + y * QTH + QTH - d->h - 2 + fy_, left ? GFX_FLIP_H : 0);
}

static void strada_card(quiz_t *q, int i, int cx, int cy, int style)
{
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    int per = 2, rows = (Q2.n + per - 1) / per; /* two arrows a row */
    for (int k = 0; k < Q2.n; k++) {
        const sprite_t *a = gfx_sprite(SARROW[Q2.opt[i][k]]);
        int r = k / per, c = k % per, inrow = imin(per, Q2.n - r * per);
        int x = cx - inrow * 16 / 2 + c * 16, y = cy - rows * 16 / 2 + r * 16;
        gfx_blit(a, x + (16 - a->w) / 2, y + (16 - a->h) / 2, 0);
    }
    gfx_set_tint(0, 0);
}

static void strada_value(quiz_t *q, int i, char *buf, size_t n) { prog_string(Q2.opt[i], Q2.n, buf, n); }

static const char *strada_subject(quiz_t *q)
{
    static char s[32];
    snprintf(s, sizeof(s), "strada:%d", Q2.n);
    return s;
}

const quiz_def_t QUIZ_STRADA = {
    .name = "sentiero",
    .game = GAME_SENTIERO,
    .bg = "bg_prato",
    .card = QUIZ_CARD_PICTURE,
    .setup = strada_setup,
    .prepare = strada_prepare,
    .tick = strada_tick,
    .ask = strada_ask,
    .hover = strada_hover,
    .hint = strada_hint,
    .draw = strada_draw,
    .card_draw = strada_card,
    .card_value = strada_value,
    .subject = strada_subject,
};
