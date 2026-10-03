/* Deva's Awesome Adventures - "Sopra e sotto": the voice asks to put the star
 * (or the ball) on the table, under it, in the box, out of it, on the roof,
 * left or right of the house. She carries it from place to place with the
 * black cross and leaves it with the red button.
 *
 * Levels: 1 table, on/under; 2 box, in/out; 3 both, with the places beside
 * the table as extra choices; 4 house (on the roof, left, right) and table
 * left/right; 5 everything. Two mistakes: the right place glows (guided
 * help). One question = one star, like the card games.
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

#define OBJ 36       /* pic_stella / pic_palla */
#define FLOOR_Y 158  /* where the things stand: the rug of her bedroom (bg_cameretta, 0.12) */
#define HOME_X 104   /* where the object waits: up by the window */
#define HOME_Y 58

enum { REF_TAVOLO, REF_SCATOLA, REF_CASA, REF_COUNT };
enum { S_HOME, S_SOPRA, S_SOTTO, S_DENTRO, S_SINISTRA, S_DESTRA, S_COUNT };
enum { T_SOPRA, T_SOTTO, T_DENTRO, T_FUORI, T_SINISTRA, T_DESTRA, T_COUNT };
enum { D_INTRO, D_PREPARE, D_INPUT, D_RIGHT, D_WRONG };

static const char *REF_IDS[REF_COUNT] = {"tavolo", "scatola", "casa"};
static const char *SPOT_IDS[S_COUNT] = {"partenza", "sopra", "sotto", "dentro", "sinistra", "destra"};
static const char *TARGET_IDS[T_COUNT] = {"sopra", "sotto", "dentro", "fuori", "sinistra", "destra"};
static const char *OBJ_IDS[2] = {"stella", "palla"};

typedef struct {
    int ref, target;
} ask_t;

static const ask_t ASK_L1[] = {{REF_TAVOLO, T_SOPRA}, {REF_TAVOLO, T_SOTTO}};
static const ask_t ASK_L2[] = {{REF_SCATOLA, T_DENTRO}, {REF_SCATOLA, T_FUORI}};
static const ask_t ASK_L3[] = {{REF_TAVOLO, T_SOPRA}, {REF_TAVOLO, T_SOTTO}, {REF_SCATOLA, T_DENTRO},
                               {REF_SCATOLA, T_FUORI}};
static const ask_t ASK_L4[] = {{REF_CASA, T_SOPRA},     {REF_CASA, T_SINISTRA},  {REF_CASA, T_DESTRA},
                               {REF_TAVOLO, T_SINISTRA}, {REF_TAVOLO, T_DESTRA}};
static const ask_t ASK_L5[] = {{REF_TAVOLO, T_SOPRA},    {REF_TAVOLO, T_SOTTO},   {REF_TAVOLO, T_SINISTRA},
                               {REF_TAVOLO, T_DESTRA},   {REF_SCATOLA, T_DENTRO}, {REF_SCATOLA, T_FUORI},
                               {REF_CASA, T_SOPRA},      {REF_CASA, T_SINISTRA},  {REF_CASA, T_DESTRA}};

static struct {
    int state, t, level;
    int ref, target, obj;
    bool active[S_COUNT];
    int sx[S_COUNT], sy[S_COUNT]; /* centre of the object in each place */
    int cur;                      /* place where the object is */
    int ox, oy;                   /* object position, 1/16 px, gliding to the place */
    int attempts, errors;
    bool assisted, ghost;
    int question, stars, idle;
    int last_ask, same_run;       /* the question before, and how many times in a row */
    int pcx;                      /* panel centre */
} D;

static bool is_right(int spot)
{
    switch (D.target) {
    case T_SOPRA: return spot == S_SOPRA;
    case T_SOTTO: return spot == S_SOTTO;
    case T_DENTRO: return spot == S_DENTRO;
    case T_FUORI: return spot == S_SINISTRA || spot == S_DESTRA;
    case T_SINISTRA: return spot == S_SINISTRA;
    case T_DESTRA: return spot == S_DESTRA;
    }
    return false;
}

static void place_spots(void)
{
    int c = D.pcx, floor_cy = FLOOR_Y - 16; /* an object standing on the floor */
    memset(D.active, 0, sizeof(D.active));
    D.active[S_HOME] = true;
    D.sx[S_HOME] = HOME_X;
    D.sy[S_HOME] = HOME_Y;
    switch (D.ref) {
    case REF_TAVOLO: /* 128x80, top surface 2 px under its top edge */
        D.active[S_SOPRA] = D.active[S_SOTTO] = true;
        D.sx[S_SOPRA] = c, D.sy[S_SOPRA] = FLOOR_Y - 80 + 1 - 15;
        D.sx[S_SOTTO] = c, D.sy[S_SOTTO] = floor_cy;
        if (D.level >= 3)
            D.active[S_SINISTRA] = D.active[S_DESTRA] = true;
        D.sx[S_SINISTRA] = c - 84, D.sx[S_DESTRA] = c + 84;
        break;
    case REF_SCATOLA: /* 104x80, the front starts 18 px under the top */
        D.active[S_DENTRO] = D.active[S_SINISTRA] = D.active[S_DESTRA] = true;
        D.sx[S_DENTRO] = c, D.sy[S_DENTRO] = FLOOR_Y - 66;
        D.sx[S_SINISTRA] = c - 72, D.sx[S_DESTRA] = c + 72;
        break;
    case REF_CASA: /* 84x84, roof top 70 px over the floor */
        D.active[S_SOPRA] = D.active[S_SINISTRA] = D.active[S_DESTRA] = true;
        D.sx[S_SOPRA] = c, D.sy[S_SOPRA] = FLOOR_Y - 71 - 15;
        D.sx[S_SINISTRA] = c - 57, D.sx[S_DESTRA] = c + 57;
        break;
    }
    D.sy[S_SINISTRA] = D.sy[S_DESTRA] = floor_cy;
}

/* the nearest place in a direction of the cross: only places within about 56
   degrees of the arrow, the ones straight ahead preferred; the waiting place in
   her hands is where the object starts, never where an arrow takes it back
   (right from there goes on the table, not to the floor beside it) */
static int nav(int from, int dx, int dy)
{
    int best = from, best_score = 1 << 30;
    for (int s = 0; s < S_COUNT; s++) {
        if (!D.active[s] || s == from || s == S_HOME)
            continue;
        int vx = D.sx[s] - D.sx[from], vy = D.sy[s] - D.sy[from];
        int along = vx * dx + vy * dy, side = absi(vx * dy - vy * dx);
        if (along <= 0 || 2 * side > 3 * along)
            continue;
        if (along + 2 * side < best_score) {
            best_score = along + 2 * side;
            best = s;
        }
    }
    return best;
}

static const int DIRS[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
static const char DIR_CH[4] = {'U', 'D', 'L', 'R'};

/* first move of the shortest way to a place that satisfies want (for the test bot) */
static char way_to(bool (*want)(int))
{
    if (want(D.cur))
        return '-';
    int first[S_COUNT], queue[S_COUNT], qh = 0, qt = 0;
    bool seen[S_COUNT] = {false};
    seen[D.cur] = true;
    for (int d = 0; d < 4; d++) {
        int s = nav(D.cur, DIRS[d][0], DIRS[d][1]);
        if (!seen[s]) {
            seen[s] = true;
            first[s] = d;
            queue[qt++] = s;
        }
    }
    while (qh < qt) {
        int s = queue[qh++];
        if (want(s))
            return DIR_CH[first[s]];
        for (int d = 0; d < 4; d++) {
            int n = nav(s, DIRS[d][0], DIRS[d][1]);
            if (!seen[n]) {
                seen[n] = true;
                first[n] = first[s];
                queue[qt++] = n;
            }
        }
    }
    return '?';
}

static bool want_wrong(int s) { return s != S_HOME && D.active[s] && !is_right(s); }

static void bot_where(const char *what)
{
    BOT("%s at=%s goal=%c other=%c\n", what, SPOT_IDS[D.cur], way_to(is_right), way_to(want_wrong));
}

static void say_question(bool queue)
{
    char id[40];
    snprintf(id, sizeof(id), "dv_%s_%s_%s", OBJ_IDS[D.obj], TARGET_IDS[D.target], REF_IDS[D.ref]);
    if (queue)
        say_then(id);
    else
        say(id);
}

static void new_question(void)
{
    D.level = clampi(G.prog.level[GAME_DOVE], 1, 5);
    const ask_t *list = ASK_L1;
    int n = ARRAY_LEN(ASK_L1);
    switch (D.level) {
    case 2: list = ASK_L2, n = ARRAY_LEN(ASK_L2); break;
    case 3: list = ASK_L3, n = ARRAY_LEN(ASK_L3); break;
    case 4: list = ASK_L4, n = ARRAY_LEN(ASK_L4); break;
    case 5: list = ASK_L5, n = ARRAY_LEN(ASK_L5); break;
    }
    /* with two questions (levels 1 and 2) the same may come twice, never three
       times - so "sopra, sotto, sopra..." is not a rule to guess, one must listen;
       with more, never the same twice */
    int k;
    do
        k = rng_range(0, n - 1);
    while (n > 1 && k == D.last_ask && (n > 2 || D.same_run >= 2));
    D.same_run = k == D.last_ask ? D.same_run + 1 : 1;
    D.last_ask = k;
    D.ref = list[k].ref;
    D.target = list[k].target;
    D.obj = rng_range(0, 1);
    place_spots();
    D.cur = S_HOME;
    D.ox = D.sx[S_HOME] * 16;
    D.oy = D.sy[S_HOME] * 16;
    D.attempts = D.errors = 0;
    D.assisted = D.ghost = false;
    D.idle = 0;
    D.state = D_PREPARE;
    D.t = 0;
    BOT("dove q=%d level=%d ref=%s target=%s obj=%s\n", D.question + 1, D.level, REF_IDS[D.ref],
        TARGET_IDS[D.target], OBJ_IDS[D.obj]);
}

static void enter(void)
{
    memset(&D, 0, sizeof(D));
    int x0 = 80, y0 = 28, x1 = 302, y1 = 166;
    gfx_anchor("panel0", &x0, &y0);
    gfx_anchor("panel1", &x1, &y1);
    D.pcx = (x0 + x1) / 2 + 8; /* a little right: the waiting place is on the left */
    D.last_ask = -1;
    hero_init(&G.hero, 30, 214);
    music_play(snd_find(SND_MUSIC, "palco"));
    if (!G.intro_done[GAME_DOVE]) {
        G.intro_done[GAME_DOVE] = true;
        say("dove_intro");
        hero_play(&G.hero, HA_WAVE, 90);
        D.state = D_INTRO;
    } else {
        new_question();
    }
}

static void enter_input(void)
{
    D.state = D_INPUT;
    D.t = 0;
    D.idle = 0;
    game_question_ready();
    bot_where("dove_ready");
}

static void log_it(int spot, bool ok)
{
    char subject[40];
    snprintf(subject, sizeof(subject), "dove:%s:%s", REF_IDS[D.ref], TARGET_IDS[D.target]);
    game_log_answer("dove", D.level, subject, TARGET_IDS[D.target], SPOT_IDS[spot], ok,
               D.attempts + 1);
    BOT("answer pick=%d ok=%d attempt=%d\n", spot, ok, D.attempts + 1);
}

static void right_answer(void)
{
    static const char *OK[] = {"ok_1", "ok_2", "ok_3", "ok_4", "ok_5", "ok_6"};
    char id[24];
    sfx("ding");
    say(OK[rng_range(0, ARRAY_LEN(OK) - 1)]);
    snprintf(id, sizeof(id), "dv_w_%s", SPOT_IDS[D.cur]); /* the word: "Sopra!" */
    say_then(D.target == T_FUORI ? "dv_w_fuori" : id);
    if (G.cfg.rumble)
        frontend_rumble(1, 10);
    hero_play(&G.hero, HA_DANCE, 100);
    fx_confetti(D.sx[D.cur], D.sy[D.cur], 36);
    fx_burst(D.sx[D.cur], D.sy[D.cur], 12);
    fx_hearts(G.hero.x + 4, G.hero.y - 70, 3);
    D.stars++;
    G.prog.stars_total++;
    int n = G.cfg.questions_per_round;
    game_star_fly(D.sx[D.cur], D.sy[D.cur], D.pcx - 8, D.stars - 1, n); /* the star flies to its place */
    sfx("star");
    if (game_level_result(GAME_DOVE, D.errors == 0 && !D.assisted, D.assisted)) {
        say_then("livello_su");
        fx_starfall(26); /* a shower of stars for the new level */
    }
    D.state = D_RIGHT;
    D.t = 0;
    input_block(30);
}

static void drop(void)
{
    if (D.cur == S_HOME) { /* still in her hands: say again where it goes */
        sfx("blip");
        say_question(false);
        return;
    }
    bool ok = is_right(D.cur);
    log_it(D.cur, ok);
    if (ok) {
        right_answer();
        return;
    }
    sfx("boop");
    D.attempts++;
    D.errors++;
    hero_play(&G.hero, HA_OH, 45);
    say(D.attempts == 1 ? voice_variant("ko_1") : "ko_2");
    D.state = D_WRONG;
    D.t = 0;
    input_block(20);
}

static void update(void)
{
    D.t++;
    /* the object glides to its place */
    int tx = D.sx[D.cur] * 16, ty = D.sy[D.cur] * 16;
    if ((absi(tx - D.ox) > 6 * 16 || absi(ty - D.oy) > 6 * 16) && (D.t & 1)) { /* a little trail of stars behind it */
        int dx, dy;
        rng_pair(-4, 4, -4, 4, &dx, &dy);
        fx_twinkle(D.ox / 16 + dx, D.oy / 16 + dy, 0);
    }
    D.ox += (tx - D.ox) / 3;
    D.oy += (ty - D.oy) / 3;
    if (absi(tx - D.ox) < 8)
        D.ox = tx;
    if (absi(ty - D.oy) < 8)
        D.oy = ty;

    switch (D.state) {
    case D_INTRO:
        if (D.t > 30 && !voice_busy())
            new_question();
        break;
    case D_PREPARE:
        if (D.t == 1) {
            sfx("pop");
            fx_sparkles(HOME_X, HOME_Y, 10, 8);
        }
        if (D.t > 24) {
            say_question(false);
            if (G.tutorial == 0)
                say_then("istruzioni");
            else if (G.tutorial == 1)
                say_then("aiuto_b");
            if (G.tutorial < 2)
                G.tutorial++;
            enter_input();
        }
        break;
    case D_INPUT: {
        bool moving = btn_held(BTN_UP) || btn_held(BTN_DOWN) || btn_held(BTN_LEFT) || btn_held(BTN_RIGHT);
        if (voice_busy() || moving)
            D.idle = 0;
        else if (++D.idle == 15 * FPS)
            say_question(false); /* distracted? ask again, once */
        for (int d = 0; d < 4; d++) {
            static const int B[4] = {BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT};
            if (!btn_pressed(B[d]))
                continue;
            int n = nav(D.cur, DIRS[d][0], DIRS[d][1]);
            if (n != D.cur) {
                D.cur = n;
                sfx("blip");
                bot_where("dove");
            } else {
                sfx("boop"); /* nothing that way */
            }
            return;
        }
        if (btn_pressed(BTN_B) || btn_pressed(BTN_Y)) {
            game_question_replay();
            say_question(false);
        }
        else if (btn_pressed(BTN_A))
            drop();
        break;
    }
    case D_WRONG:
        if (D.t == 26) { /* back to her hands */
            D.cur = S_HOME;
            sfx("whoosh");
        }
        if (D.t > 40 && !voice_busy()) {
            if (D.attempts >= 2 && !D.ghost) {
                /* guided help: the right place glows, the voice says it again */
                D.ghost = true;
                D.assisted = true;
                say("dv_guarda");
                say_question(true);
                BOT("hint\n");
            }
            enter_input();
        }
        break;
    case D_RIGHT:
        if (D.t > 100 && !voice_busy()) {
            if (++D.question >= G.cfg.questions_per_round)
                game_round_done(GAME_DOVE);
            else
                new_question();
        }
        break;
    }
}

static void draw_object(int x, int y, bool ghost)
{
    char name[24];
    snprintf(name, sizeof(name), "pic_%s", OBJ_IDS[D.obj]);
    const sprite_t *s = gfx_sprite(name);
    if (ghost)
        gfx_set_tint(rgb565(0xff, 0xf4, 0xfa), 150);
    gfx_blit(s, x - OBJ / 2, y - OBJ / 2, 0);
    if (ghost)
        gfx_set_tint(0, 0);
}

static void draw_ref(bool front_only)
{
    int c = D.pcx;
    switch (D.ref) {
    case REF_TAVOLO:
        if (!front_only)
            gfx_blit_scaled(gfx_sprite("tavolo"), c - 64, FLOOR_Y - 80, 2, 0);
        break;
    case REF_SCATOLA:
        gfx_blit_scaled(gfx_sprite(front_only ? "scatola_fronte" : "scatola"), c - 52, FLOOR_Y - 80, 2, 0);
        break;
    case REF_CASA:
        if (!front_only)
            gfx_blit(gfx_sprite("casa_grande"), c - 42, FLOOR_Y - 78, 0);
        break;
    }
}

static void draw(void)
{
    amb_bg("bg_cameretta");
    game_draw_stars(D.pcx - 8, D.stars, G.cfg.questions_per_round);
    if (D.state != D_INTRO) {
        draw_ref(false);
        /* the places where it can go: soft marks on the floor, the current one darker */
        const sprite_t *sh = gfx_sprite("shadow");
        for (int s = 1; s < S_COUNT; s++) {
            if (!D.active[s] || (s == D.cur && D.state == D_INPUT))
                continue;
            if (D.ref == REF_SCATOLA && s == S_DENTRO)
                continue; /* inside the box: no mark, the box says it */
            gfx_set_tint(rgb565(0xff, 0xf4, 0xfa), 110);
            gfx_blit(sh, D.sx[s] - sh->w / 2, D.sy[s] + 14, 0);
            gfx_set_tint(0, 0);
        }
        if (D.ghost && D.state == D_INPUT && ((G.frame / 12) & 1))
            for (int s = 1; s < S_COUNT; s++)
                if (D.active[s] && is_right(s))
                    draw_object(D.sx[s], D.sy[s], true);
        int x = D.ox / 16, y = D.oy / 16;
        bool carried = D.state == D_INPUT || D.state == D_PREPARE;
        int bob = carried ? -swing((int)G.frame, 36, 2) : 0;
        if (D.state == D_RIGHT && D.t < 40)
            bob = -((D.t / 5) & 1) * 3;
        if (carried && D.cur != S_HOME && !(D.ref == REF_SCATOLA && D.cur == S_DENTRO))
            gfx_blit(sh, D.sx[D.cur] - sh->w / 2, D.sy[D.cur] + 14, 0);
        bool in_box = D.ref == REF_SCATOLA && D.cur == S_DENTRO && absi(x - D.sx[S_DENTRO]) < 30 &&
                      absi(y - D.sy[S_DENTRO]) < 24;
        draw_object(x, y + bob, false);
        if (in_box)
            draw_ref(true);
        if (D.state == D_INPUT && D.t > 240 && ((D.t / 30) & 1))
            gfx_blit(gfx_sprite("btn_a"), x + 14, y - 34, 0); /* nudge: the red button */
    }
    hero_draw(&G.hero, &G.prog);
    int hl = hud_tutorial_highlight();
    if (hl != HUD_NONE)
        hud_console(0, 58, hl, ((G.frame / 10) & 1) == 0);
    fx_draw();
}

const scene_t SCENE_DOVE = {enter, update, draw};
