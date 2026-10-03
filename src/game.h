/* Deva's Awesome Adventures - game state, input and scene switching.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_GAME_H
#define DEVA_GAME_H

#include "common.h"
#include "hero.h"
#include "save.h"

enum { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_X, BTN_Y, BTN_START, BTN_SELECT, BTN_L, BTN_R, BTN_COUNT };

typedef enum {
    SC_TITLE, SC_PROVA, SC_MENU, SC_CONTA, SC_PAROLE, SC_SEQUENZE, SC_BALLA, SC_NOME, SC_MEMORY, SC_RITMO,
    SC_DOVE, SC_EMOZIONI, SC_STORIE, SC_MAPPA, SC_RACCONTO, SC_SFIDA, SC_PREMIO, SC_FINE,
    SC_SALVATAGGI, SC_TASTIERA, SC_OPZIONI, SC_CAMERINO, SC_ALBUM, SC_GINNASTICA, SC_TRUCCO, SC_FORME,
    SC_LETTERE, SC_OMBRE, SC_SENTIERO, SC_NEGOZIO, SC_MISURE, SC_COUNT
} scene_id_t;

typedef struct {
    void (*enter)(void);
    void (*update)(void);
    void (*draw)(void);
} scene_t;

typedef struct {
    config_t cfg;
    config_t cfg_base;               /* the parent's .cfg file: the in-game options save what differs */
    int slot;                        /* profile in use, 1..PROFILE_COUNT */
    progress_t prog;
    hero_t hero;
    scene_id_t scene;
    uint32_t frame;                  /* frames since the core started */
    uint32_t session_frames;         /* play time, excluding the ending */
    int tutorial;                    /* spoken tips given so far this session (0..2) */
    bool intro_done[GAME_COUNT];     /* each game introduces itself once per session */
    int rounds_session[GAME_COUNT];  /* rounds played this session, per game */
    int last_game;                   /* game of the round that opened the dressing room */
    bool from_round;                 /* PREMIO entered after a completed round */
    bool just_charged;               /* the last round charged the wand of the tale (menu shows it) */
    int map_event;                   /* MAP_* for the next visit of the map */
    bool map_told;                   /* the map has said who is next, this session */
    bool prova_done;                 /* the controls tutorial is done, this session */
    bool quitting;
    uint32_t play_frames;            /* play time not yet added to prog.play_seconds */
    int edit_slot;                   /* the name editor: profile being named */
    bool edit_new;                   /* ...a new one (else renamed) */
    int title_sel;                   /* main menu: the item to highlight when it opens */
    bool last_told;                  /* "ancora un gioco, e poi la nanna", said this session (0.14) */
    uint32_t q_ready;                /* the parent log (0.15): frame the question could be answered from */
    bool q_known;                    /* ...set by the game for the answer to come */
    int q_replays;                   /* the question asked again on request, since the last answer */
} game_t;

extern game_t G;

bool game_init(void);
void game_frame(void);
void game_goto(scene_id_t s);
void game_shutdown(void);
bool session_over(void);
bool session_ending(void); /* less than a round left: the next game is the last one (0.14) */
/* the parent log (0.15): the games say when a question can be answered (the time to answer starts)
   and when it is asked again on request; the answer row carries both */
void game_question_ready(void);
void game_question_replay(void);

/* shared game rules */
scene_id_t game_scene(int game);              /* GAME_* -> scene */
void game_round_done(int game);               /* bookkeeping, save, open the dressing room */
bool game_level_result(int game, bool first_try, bool assisted); /* true = level went up */
void game_draw_stars(int cx, int stars, int total);
void game_star_fly(int x, int y, int cx, int index, int total); /* a star won flies from (x, y) to its place */

void input_set(uint32_t mask); /* bit per BTN_* */
bool btn_pressed(int b);
bool btn_held(int b);
void input_block(int frames);  /* ignore presses for a while (feedback, transitions) */
/* the grown-ups' gesture (0.14): L and R held down together. A child presses one button, holds the red
   one, mashes them all - two shoulder buttons held at once for two seconds she does not find by chance */
bool grownup_held(void);
bool btn_any_pressed(void); /* any button, this frame */
/* an idle reminder that does not nag (0.14): due after `base` frames of quiet, then after twice and four
   times as long, then not any more until a button is pressed. quiet: frames without the voice (the
   caller counts them), nags: reminders given since the last press */
bool idle_reminder(int quiet, int *nags, int base);
/* a line said often, with its variants in the voice folder (<id>, <id>_b, <id>_c...): a different one
   each time, never the same twice in a row (0.14) */
const char *voice_variant(const char *id);

/* scene tables, one per file */
extern const scene_t SCENE_TITLE, SCENE_PROVA, SCENE_MENU, SCENE_CONTA, SCENE_PAROLE, SCENE_SEQUENZE,
    SCENE_BALLA, SCENE_NOME, SCENE_MEMORY, SCENE_RITMO, SCENE_DOVE, SCENE_EMOZIONI, SCENE_STORIE, SCENE_MAPPA,
    SCENE_RACCONTO, SCENE_SFIDA, SCENE_PREMIO, SCENE_FINE, SCENE_SALVATAGGI, SCENE_TASTIERA, SCENE_OPZIONI,
    SCENE_CAMERINO, SCENE_ALBUM, SCENE_GINNASTICA, SCENE_TRUCCO, SCENE_FORME, SCENE_LETTERE, SCENE_OMBRE,
    SCENE_SENTIERO, SCENE_NEGOZIO, SCENE_MISURE;
bool prova_wanted(void); /* controls tutorial: first sessions only */
void game_home(void);    /* after the title: the map of the tale, or the menu */
void game_play(void);    /* "Gioca": the controls tutorial when due, else game_home() */

/* profiles and saving */
void game_save(void);                               /* the profile in use (after rounds, rewards...) */
void game_save_options(void);                       /* the in-game options and the profile in use */
void game_use_profile(int slot);                    /* switch to an existing profile */
void game_new_profile(int slot, const char *name);  /* create a profile and play with it */
void game_rename_profile(int slot, const char *name);
void game_delete_profile(int slot);                 /* the one in use starts again from scratch */
const char *child_name(void);                       /* name of the child of this profile */
bool name_voice(const char *id);                    /* a clip with the name in it, for this child */
void game_log_answer(const char *game, int level, const char *subject, const char *target,
                     const char *chosen, bool correct, int attempt);

/* pause (START or SELECT held down for a moment during play: a tap by chance does nothing): drawn over
   the frozen scene */
bool game_paused(void);

/* debug hook for the automated tests (see tools/harness) */
#define BOT(...) log_msg(LOG_DEBUG, "BOT " __VA_ARGS__)

#endif
