/* Deva's Awesome Adventures - game state, input edges, scene switching, profiles,
 * the pause, and the rules shared by every mini-game (rounds, stars, adaptive levels).
 * SPDX-License-Identifier: MIT
 */
#include "game.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "gfx.h"
#include "story.h"
#include "trans.h"
#include "ui.h"

static bool s_shown; /* false: the first scene after loading - a star opens the show on the black screen */

game_t G;

static uint32_t s_now, s_prev;
static int s_block;
static const scene_t *SCENES[SC_COUNT];
static const char *SCENE_NAMES[SC_COUNT] = {
    "title",  "prova",    "menu",     "conta",    "parole",     "sequenze", "balla",    "nome",
    "memory", "ritmo",    "dove",     "emozioni", "storie",     "mappa",    "racconto", "sfida",
    "premio", "fine",     "salvataggi", "tastiera", "opzioni",  "camerino", "album",    "ginnastica",
    "trucco", "forme",    "lettere",  "ombre",    "sentiero",   "negozio",  "misure"};

/* the session diary of the profile in use (0.15): for the grown-ups who watch the first sessions */
static void diary(const char *event, const char *detail)
{
    char path[512];
    profile_diary_path(path, sizeof(path), dir_save(), G.slot);
    log_diary(path, G.prog.sessions, (long)(G.frame / FPS), event, detail);
}

void input_set(uint32_t mask)
{
    s_prev = s_now;
    s_now = mask;
    if (s_block > 0)
        s_block--;
}

bool btn_held(int b) { return (s_now >> b) & 1; }
bool grownup_held(void) { return btn_held(BTN_L) && btn_held(BTN_R); }
bool btn_any_pressed(void) { return s_block == 0 && (s_now & ~s_prev) != 0; }

bool idle_reminder(int quiet, int *nags, int base)
{
    if (btn_any_pressed())
        *nags = 0;
    if (*nags >= 3 || quiet <= (base << *nags))
        return false;
    ++*nags;
    diary("promemoria", SCENE_NAMES[G.scene]);
    return true;
}

const char *voice_variant(const char *id)
{
    static struct {
        char id[24];
        int last;
        char out[32];
    } mem[8];
    static int next_slot;
    int m = -1;
    for (int i = 0; i < ARRAY_LEN(mem); i++)
        if (!strcmp(mem[i].id, id)) {
            m = i;
            break;
        }
    if (m < 0) { /* a new line: take a slot (the oldest goes) */
        m = next_slot;
        next_slot = (next_slot + 1) % ARRAY_LEN(mem);
        snprintf(mem[m].id, sizeof(mem[m].id), "%s", id);
        mem[m].last = -1;
    }
    char name[32];
    int n = 1;
    while (n < 8) {
        snprintf(name, sizeof(name), "%s_%c", id, 'a' + n);
        if (!snd_exists(SND_VOICE, name))
            break;
        n++;
    }
    int k = rng_range(0, n - 1);
    if (n > 1 && k == mem[m].last)
        k = (k + 1 + rng_range(0, n - 2)) % n;
    mem[m].last = k;
    if (k == 0)
        snprintf(mem[m].out, sizeof(mem[m].out), "%s", id);
    else
        snprintf(mem[m].out, sizeof(mem[m].out), "%s_%c", id, 'a' + k);
    return mem[m].out;
}
bool btn_pressed(int b) { return s_block == 0 && ((s_now & ~s_prev) >> b) & 1; }
void input_block(int frames) { s_block = frames > s_block ? frames : s_block; }

void game_home(void)
{
    if (story_epilogue_due()) /* the fourth adventure ended last time: the party for Deva (0.12) */
        racconto_play(R_EPILOGO, SC_MENU);
    else if (story_active())
        mappa_open(MAP_PLAIN);
    else if (G.cfg.story && G.prog.chapter >= CH_COUNT && !story_seen(SEEN_FINALE))
        racconto_play(R_FINALE, SC_MENU); /* an ending cut short (the console turned off): told again */
    else
        game_goto(SC_MENU);
}

void game_play(void)
{
    if (prova_wanted())
        game_goto(SC_PROVA);
    else
        game_home();
}

bool session_over(void)
{
    return G.cfg.session_minutes > 0 && G.session_frames >= (uint32_t)G.cfg.session_minutes * 60u * FPS;
}

bool session_ending(void)
{
    uint32_t total = (uint32_t)G.cfg.session_minutes * 60u * FPS;
    return G.cfg.session_minutes > 0 && G.session_frames + 150u * FPS >= total && G.session_frames < total;
}

/* ------------------------------------------------------------------ profiles */

const char *child_name(void) { return G.prog.name[0] ? G.prog.name : G.cfg.child_name; }

bool name_voice(const char *id)
{
    /* the clips with a name were made for the name in the parent's .cfg */
    return !strcmp(child_name(), G.cfg.child_name) && snd_exists(SND_VOICE, id);
}

void game_save(void)
{
    G.prog.play_seconds += G.play_frames / FPS;
    G.play_frames %= FPS;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm)
        strftime(G.prog.last_day, sizeof(G.prog.last_day), "%Y-%m-%d", tm);
    progress_save(&G.prog, dir_save(), G.slot);
}

void game_save_options(void) { options_save(&G.cfg, &G.cfg_base, G.slot, dir_save()); }

static void new_session_state(void)
{
    memset(G.intro_done, 0, sizeof(G.intro_done));
    memset(G.rounds_session, 0, sizeof(G.rounds_session));
    G.tutorial = 0;
    G.last_game = GAME_COUNT - 1; /* so the menu suggests the first game first */
    G.just_charged = false;
    G.from_round = false;
    G.map_told = false;
    G.prova_done = false;
}

static uint32_t s_counted; /* profiles whose session is already counted since the core started */

static void load_profile(int slot)
{
    G.slot = clampi(slot, 1, PROFILE_COUNT);
    if (!progress_load(&G.prog, dir_save(), G.slot, G.cfg.start_level))
        log_msg(LOG_INFO, "profile %d: no saved progress yet\n", G.slot);
    if (!G.prog.name[0] && G.slot == 1) /* files of older versions: the name of the .cfg */
        snprintf(G.prog.name, sizeof(G.prog.name), "%s", G.cfg.child_name);
    if (!(s_counted & (1u << G.slot))) { /* switching back and forth is still one session */
        s_counted |= 1u << G.slot;
        G.prog.sessions++;
        story_session_start(); /* a tale that ended last time: the next adventure begins */
    }
    new_session_state();
    game_save();
    char what[48];
    snprintf(what, sizeof(what), "versione %s, profilo %d", GAME_VERSION, G.slot);
    diary("inizio", what);
    BOT("profile slot=%d name=%s sessions=%d\n", G.slot, G.prog.name, G.prog.sessions);
}

void game_use_profile(int slot)
{
    if (slot == G.slot)
        return;
    game_save();
    char what[8];
    snprintf(what, sizeof(what), "%d", slot);
    diary("cambio_profilo", what);
    load_profile(slot);
    game_save_options();
}

void game_new_profile(int slot, const char *name)
{
    game_save();
    G.slot = clampi(slot, 1, PROFILE_COUNT);
    progress_reset(&G.prog, G.cfg.start_level);
    set_name(G.prog.name, sizeof(G.prog.name), name);
    G.prog.sessions = 1;
    s_counted |= 1u << G.slot;
    new_session_state();
    game_save();
    game_save_options();
    BOT("profile new slot=%d name=%s\n", G.slot, G.prog.name);
}

void game_rename_profile(int slot, const char *name)
{
    if (slot == G.slot) {
        set_name(G.prog.name, sizeof(G.prog.name), name);
        game_save();
    } else {
        progress_t p;
        if (!progress_load(&p, dir_save(), slot, G.cfg.start_level))
            return;
        set_name(p.name, sizeof(p.name), name);
        progress_save(&p, dir_save(), slot);
    }
    BOT("profile rename slot=%d name=%s\n", slot, name);
}

void game_delete_profile(int slot)
{
    profile_remove(dir_save(), slot); /* a .bak copy stays, just in case */
    if (slot == G.slot) {             /* the one in use: a fresh start with the same name */
        char name[16];
        snprintf(name, sizeof(name), "%s", child_name());
        progress_reset(&G.prog, G.cfg.start_level);
        snprintf(G.prog.name, sizeof(G.prog.name), "%s", name);
        G.prog.sessions = 1;
        new_session_state();
        game_save();
    }
    BOT("profile delete slot=%d\n", slot);
}

void game_log_answer(const char *game, int level, const char *subject, const char *target, const char *chosen,
                     bool correct, int attempt)
{
    for (int g = 0; g < GAME_COUNT; g++)
        if (!strcmp(GAME_IDS[g], game) && attempt <= 1) {
            G.prog.answers[g]++;
            if (correct)
                G.prog.first_try[g]++;
        }
    char path[512];
    profile_log_path(path, sizeof(path), dir_save(), G.slot);
    log_answer(path, G.prog.sessions, game, level, subject, target, chosen, correct, attempt,
               G.q_known ? (double)(G.frame - G.q_ready) / FPS : -1.0, G.q_replays, (long)(G.frame / FPS));
    G.q_known = false; /* the next try is timed from its own start */
    G.q_replays = 0;
}

void game_question_ready(void)
{
    G.q_ready = G.frame;
    G.q_known = true;
}

void game_question_replay(void) { G.q_replays++; }

/* ------------------------------------------------------------------ pause */

enum { PA_CONTINUE, PA_HOME, PA_TITLE };

static struct {
    bool on;
    int items[3], n, sel, t;
} P;

bool game_paused(void) { return P.on; }

/* the menu, the games, the map, the tales and the duels; not the controls
   tutorial (START is one of the buttons it names wrong) nor the dressing room
   after a round (the reward would be lost) */
static bool pausable(scene_id_t s)
{
    return (s >= SC_MENU && s <= SC_SFIDA) || (s >= SC_GINNASTICA && s <= SC_MISURE);
}

static void pause_say(void)
{
    static const char *IDS[] = {"p_continua", "p_giochi", "p_inizio"};
    int it = P.items[P.sel];
    say(it == PA_HOME && story_active() ? "p_mappa" : IDS[it]);
}

static void pause_open(void)
{
    P.on = true;
    P.n = 0;
    P.items[P.n++] = PA_CONTINUE;
    /* in a game or a duel: back to the games or the map (a tale is not left
       halfway for the map: it goes on, or back to the main menu) */
    if (G.scene != SC_MENU && G.scene != SC_MAPPA && G.scene != SC_RACCONTO)
        P.items[P.n++] = PA_HOME;
    P.items[P.n++] = PA_TITLE;
    P.sel = 0;
    P.t = 0;
    voice_hold();
    sfx("blip");
    say("pausa");
    input_block(10);
    diary("pausa", "aperta");
    BOT("pause open n=%d\n", P.n);
}

static void pause_close(int choice)
{
    P.on = false;
    diary("pausa", choice == PA_CONTINUE ? "continua" : choice == PA_HOME ? "giochi" : "inizio");
    BOT("pause choice=%d\n", choice);
    if (choice == PA_CONTINUE) {
        voice_resume(true);
        input_block(10);
        return;
    }
    voice_resume(false);
    G.from_round = false;
    if (choice == PA_HOME)
        game_home();
    else
        game_goto(SC_TITLE);
}

static void pause_update(void)
{
    P.t++;
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir && P.sel + dir >= 0 && P.sel + dir < P.n) {
        P.sel += dir;
        sfx("blip");
        pause_say();
        BOT("pause sel=%d\n", P.sel);
    } else if (btn_pressed(BTN_A)) {
        sfx("star");
        pause_close(P.items[P.sel]);
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_START) || btn_pressed(BTN_SELECT)) {
        pause_close(PA_CONTINUE);
    }
}

static void pause_draw(void)
{
    static const char *ICONS[] = {"mm_gioca", "mm_giochi", "mm_inizio"};
    static const char *LABELS[] = {"Continua", "Torna ai giochi", "Menu principale"};
    gfx_reset_state();
    gfx_fade(rgb565(0x24, 0x12, 0x2f), 200);
    /* a panel: the frozen game stays in the background */
    gfx_round_rect(26, 39, 272, 150, rgb565(0x24, 0x12, 0x2f), rgb565(0x24, 0x12, 0x2f));
    gfx_round_rect(24, 36, 272, 150, rgb565(0xea, 0xdc, 0xfc), rgb565(0x3b, 0x1f, 0x4a));
    gfx_text_big("PAUSA", 160, 56, NUM_SELECTED, 1);
    int step = 76, x0 = 160 - (P.n - 1) * step / 2;
    for (int i = 0; i < P.n; i++) {
        int cx = x0 + i * step, y = 76, it = P.items[i];
        const char *icon = ICONS[it];
        if (it == PA_HOME && story_active())
            icon = "mm_mappa";
        if (i == P.sel) {
            int bob = -((P.t / 16) & 1) * 2;
            gfx_blit(gfx_sprite("mcard_sel"), cx - 32, y - 2 + bob, 0);
            gfx_blit(gfx_sprite(icon), cx - 22, y + 8 + bob, 0);
            ui_glow(cx - 33, y - 3 + bob, 65, 65);
        } else {
            gfx_blit(gfx_sprite("mcard"), cx - 30, y, 0);
            gfx_blit(gfx_sprite(icon), cx - 22, y + 8, 0);
        }
    }
    const char *label = LABELS[P.items[P.sel]];
    if (P.items[P.sel] == PA_HOME && story_active())
        label = "Torna alla mappa";
    gfx_text(label, 160, 150, rgb565(0x3b, 0x1f, 0x4a), ALIGN_CENTER);
    gfx_text("A: scegli   B o START: continua", 160, 170, rgb565(0x7c, 0x5f, 0xbb), ALIGN_CENTER);
}

/* ------------------------------------------------------------------ frame */

bool game_init(void)
{
    SCENES[SC_TITLE] = &SCENE_TITLE;
    SCENES[SC_PROVA] = &SCENE_PROVA;
    SCENES[SC_MENU] = &SCENE_MENU;
    SCENES[SC_CONTA] = &SCENE_CONTA;
    SCENES[SC_PAROLE] = &SCENE_PAROLE;
    SCENES[SC_SEQUENZE] = &SCENE_SEQUENZE;
    SCENES[SC_BALLA] = &SCENE_BALLA;
    SCENES[SC_NOME] = &SCENE_NOME;
    SCENES[SC_MEMORY] = &SCENE_MEMORY;
    SCENES[SC_RITMO] = &SCENE_RITMO;
    SCENES[SC_DOVE] = &SCENE_DOVE;
    SCENES[SC_EMOZIONI] = &SCENE_EMOZIONI;
    SCENES[SC_STORIE] = &SCENE_STORIE;
    SCENES[SC_MAPPA] = &SCENE_MAPPA;
    SCENES[SC_RACCONTO] = &SCENE_RACCONTO;
    SCENES[SC_SFIDA] = &SCENE_SFIDA;
    SCENES[SC_PREMIO] = &SCENE_PREMIO;
    SCENES[SC_FINE] = &SCENE_FINE;
    SCENES[SC_SALVATAGGI] = &SCENE_SALVATAGGI;
    SCENES[SC_TASTIERA] = &SCENE_TASTIERA;
    SCENES[SC_OPZIONI] = &SCENE_OPZIONI;
    SCENES[SC_CAMERINO] = &SCENE_CAMERINO;
    SCENES[SC_ALBUM] = &SCENE_ALBUM;
    SCENES[SC_GINNASTICA] = &SCENE_GINNASTICA;
    SCENES[SC_TRUCCO] = &SCENE_TRUCCO;
    SCENES[SC_FORME] = &SCENE_FORME;
    SCENES[SC_LETTERE] = &SCENE_LETTERE;
    SCENES[SC_OMBRE] = &SCENE_OMBRE;
    SCENES[SC_SENTIERO] = &SCENE_SENTIERO;
    SCENES[SC_NEGOZIO] = &SCENE_NEGOZIO;
    SCENES[SC_MISURE] = &SCENE_MISURE;

    memset(&G, 0, sizeof(G));
    memset(&P, 0, sizeof(P));
    s_counted = 0;
    trans_init();
    s_shown = false;
    memset(g_fb, 0, sizeof(g_fb)); /* (after a restart of the content, not the last picture of before) */
    config_load(&G.cfg_base, dir_data());
    G.cfg = G.cfg_base;
    int slot = 1;
    options_load(&G.cfg, &slot, dir_save());
    if (slot != 1 && !profile_exists(dir_save(), slot))
        slot = 1;
    audio_set_volumes(G.cfg.vol_music, G.cfg.vol_voice, G.cfg.vol_sfx);
    load_profile(slot);
    hero_init(&G.hero, 160, 214);
    fx_clear();
    game_goto(SC_TITLE);
    return true;
}

/* how a scene opens: a heart for the games, a star for the tale, a rainbow for
   the dressing room after a round, a circle for goodnight, sparkles elsewhere */
static int trans_style(scene_id_t s)
{
    switch (s) {
    case SC_RACCONTO:
    case SC_MAPPA:
    case SC_SFIDA: return TR_STAR;
    case SC_PREMIO: return TR_RAINBOW;
    case SC_FINE: return TR_CIRCLE;
    case SC_TITLE:
    case SC_PROVA:
    case SC_MENU:
    case SC_SALVATAGGI:
    case SC_TASTIERA:
    case SC_OPZIONI:
    case SC_CAMERINO:
    case SC_ALBUM: return TR_SPARKLE;
    default: return TR_HEART; /* a game */
    }
}

void game_goto(scene_id_t s)
{
    if (P.on) { /* a reset of the frontend during the pause */
        P.on = false;
        voice_resume(false);
    }
    /* time is up: whatever the way back to the games (a round, the end of a
       tale, the map, the pause), the next stop is goodnight */
    if (s == SC_MENU && session_over())
        s = SC_FINE;
    if (s != SC_RACCONTO) /* a tale told again from the album ends with any other scene (the pause, too) */
        racconto_replay_cancel();
    trans_begin(s_shown ? trans_style(s) : TR_STAR); /* the new scene opens over the last frame of this one */
    s_shown = true;
    G.scene = s;
    fx_clear();
    gfx_reset_state();
    input_block(12);
    G.q_known = false; /* a question left halfway: nothing of it goes to the next answer */
    G.q_replays = 0;
    diary("scena", SCENE_NAMES[s]);
    BOT("scene=%s\n", SCENE_NAMES[s]);
    SCENES[s]->enter();
}

#define PAUSE_HOLD 40 /* frames START must stay down for the pause */
static int s_pause_hold; /* frames START (or SELECT) has been down, -1 = wait until it is up */

static bool grown_up_scene(scene_id_t s) { return s == SC_OPZIONI || s == SC_SALVATAGGI || s == SC_TASTIERA; }

void game_frame(void)
{
    G.frame++;
    audio_prefetch_step();
    if (G.frame > 2 && (G.frame & 7) == 0) /* the story's backgrounds, one now and then */
        gfx_prefetch_step();
    /* the pause opens when START (or SELECT) stays down for a moment: a pause wanted, not a button hit
       by chance (0.14: a small child with the console opened it twice a minute) */
    bool start_down = (btn_held(BTN_START) || btn_held(BTN_SELECT)) && /* alone: not a hand squeezing them all */
                      !btn_held(BTN_A) && !btn_held(BTN_B) && !btn_held(BTN_X) && !btn_held(BTN_Y);
    if (P.on || !pausable(G.scene) || G.quitting)
        s_pause_hold = start_down ? -1 : 0; /* still down after the pause: wait for it to come up */
    else if (s_pause_hold < 0)
        s_pause_hold = start_down ? -1 : 0;
    else if (start_down && ++s_pause_hold >= PAUSE_HOLD) {
        s_pause_hold = -1;
        pause_open();
    } else if (!start_down) {
        s_pause_hold = 0;
    }
    if (P.on) { /* the scene stays as it is, under the pause */
        pause_update();
        if (P.on) {
            SCENES[G.scene]->draw();
            pause_draw();
            return;
        }
    }
    if (G.scene != SC_FINE && !grown_up_scene(G.scene)) {
        G.session_frames++; /* the time limit counts the child's time */
        G.play_frames++;
    }
    G.hero.talking = voice_busy();
    SCENES[G.scene]->update();
    hero_update(&G.hero);
    fx_update();
    SCENES[G.scene]->draw();
    if (s_pause_hold > 8) { /* START held: a pause sign fills up with stars, then the pause */
        int x = SCREEN_W - 30, y = 6;
        gfx_round_rect(x, y, 22, 22, COL_CREAM, COL_INK);
        gfx_fill_rect(x + 7, y + 6, 3, 10, COL_INK);
        gfx_fill_rect(x + 12, y + 6, 3, 10, COL_INK);
        ui_hold_stars(x - 3, y - 3, 28, 28, s_pause_hold, PAUSE_HOLD);
    }
    trans_draw();
}

void game_shutdown(void)
{
    game_save(); /* idempotent: also saved after each round */
    diary("chiusa", "");
    save_wait(); /* the last save is on the card before the core goes */
}

scene_id_t game_scene(int game)
{
    static const scene_id_t MAP[GAME_COUNT] = {SC_CONTA,    SC_PAROLE,    SC_SEQUENZE, SC_BALLA,  SC_NOME,
                                               SC_MEMORY,   SC_RITMO,     SC_DOVE,     SC_EMOZIONI, SC_STORIE,
                                               SC_GINNASTICA, SC_TRUCCO, SC_FORME,   SC_LETTERE,  SC_OMBRE,
                                               SC_SENTIERO, SC_NEGOZIO, SC_MISURE};
    return MAP[clampi(game, 0, GAME_COUNT - 1)];
}

void game_round_done(int game)
{
    G.prog.rounds_total++;
    G.prog.rounds[game]++;
    G.rounds_session[game]++;
    G.last_game = game;
    G.from_round = true;
    G.prog.unlocked |= 1u << game; /* played = open (all games open: the save remembers them) */
    story_round_done();            /* every round charges the magic wand of the tale */
    /* a new game every few rounds: the menu shows it after the dressing room */
    if (G.cfg.unlock_gradual && G.prog.rounds_total % G.cfg.rounds_per_unlock == 0) {
        int g = progress_unlock_next(&G.prog);
        if (g >= 0) {
            G.prog.reveal = g; /* saved: if the session ends now, the menu shows it next time */
            BOT("unlock game=%s\n", GAME_IDS[g]);
        }
    }
    game_save();
    game_goto(SC_PREMIO);
}

bool game_level_result(int game, bool first_try, bool assisted)
{
    int *lvl = &G.prog.level[game], *streak = &G.prog.streak[game];
    if (first_try) {
        if (++*streak >= 3 && *lvl < 5) {
            ++*lvl;
            *streak = 0;
            BOT("level_up game=%s level=%d\n", GAME_IDS[game], *lvl);
            return true;
        }
    } else {
        *streak = 0;
        if (assisted && *lvl > 1) {
            --*lvl;
            BOT("level_down game=%s level=%d\n", GAME_IDS[game], *lvl);
        }
    }
    return false;
}

/* the row of stars at the top: a star won flies there (fx_fly_star) and lights
   its place when it lands, with a little hop */
void game_draw_stars(int cx, int stars, int total)
{
    int x0 = cx - total * 9, shown = stars - fx_stars_flying(), pop = fx_star_landed_age();
    for (int i = 0; i < total; i++) {
        bool on = i < shown;
        const sprite_t *s = gfx_sprite(on ? "star_on" : "star_off");
        int dy = 0, sdy = 0, sdx = 0;
        if (on && i == shown - 1 && pop < 16) { /* 0.13: it lands squashed, springs up, settles */
            sdy = pop < 4 ? -(4 - pop) : (pop < 10 ? (10 - pop) / 2 : 0);
            sdx = -sdy;
            dy = pop >= 4 && pop < 12 ? -swing(pop - 4, 16, 3) : 0;
        } else if (on) { /* the stars won breathe a little, one after the other */
            sdy = swing((int)G.frame + i * 20, 120, 1);
        }
        gfx_blit_squash(s, x0 + i * 18 + 1 + s->w / 2, 4 + dy + s->h, 1, 0, s->h / 2, sdy, sdx);
    }
}

void game_star_fly(int x, int y, int cx, int index, int total)
{
    fx_fly_star(x, y, cx - total * 9 + index * 18 + 9, 12);
}
