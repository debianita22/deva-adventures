/* Deva's Awesome Adventures - parent settings, saved profiles and the parent's answer log.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_SAVE_H
#define DEVA_SAVE_H

#include "common.h"

/* The mini-games, in menu order; each keeps its own adaptive level (1..5).
 * The first GAME_BASE are open from the start, the others unlock one by one
 * as rounds are played (or all at once, see config "sblocco_giochi"). */
enum {
    GAME_CONTA, GAME_PAROLE, GAME_SEQUENZE, GAME_BALLA, GAME_NOME, GAME_MEMORY, GAME_RITMO, GAME_DOVE,
    GAME_EMOZIONI, GAME_STORIE, GAME_GINNASTICA, GAME_TRUCCO, GAME_FORME,
    GAME_LETTERE, GAME_OMBRE, GAME_SENTIERO, /* 0.10.0 */
    GAME_NEGOZIO, GAME_MISURE,               /* 0.11.0 */
    GAME_COUNT
};
#define GAME_BASE 4
extern const char *const GAME_IDS[GAME_COUNT];   /* "conta", "parole", ... */
extern const char *const GAME_TITLES[GAME_COUNT]; /* "Conta", "Parole", ... (grown-up screens) */
extern const char *const GAME_ICONS[GAME_COUNT];  /* "menu_conta", ... */

/* Make-up items, in the order used for the "owned" bitmask. */
typedef enum { SLOT_EYES, SLOT_LIPS, SLOT_BLUSH, SLOT_STICKER, SLOT_GLITTER, SLOT_HEAD } slot_t;

typedef struct {
    const char *id;      /* voice "t_<id>", icon "rw_<id>" */
    slot_t slot;
    const char *variant; /* sprite suffix for the slot, NULL for on/off items */
} item_t;

extern const item_t ITEMS[];
extern const int ITEM_COUNT;

/* Stickers for the album, the rewards once every make-up item is won: the
 * pictures of the word games (sprite "pic_<id>", voice "w_<id>"). */
#define STICKER_COUNT 42
extern const char *const STICKERS[STICKER_COUNT];

/* Profiles: three children (or three fresh starts) on one console.
 * Slot 1 is "deva_adventures.sav" (files of older versions are slot 1),
 * slots 2 and 3 are "deva_adventures_2.sav", "deva_adventures_3.sav";
 * each has its own answer log "..._log.csv". */
#define PROFILE_COUNT 3
#define NAME_MAX_LEN 10

typedef struct {
    char name[16];          /* the child of this profile, A-Z; "" = not set (slot 1 of old files) */
    int level[GAME_COUNT];  /* 1..5 */
    int streak[GAME_COUNT]; /* consecutive first-try answers */
    uint32_t owned;         /* bit i = ITEMS[i] won */
    int worn[6];            /* per slot: item index or -1 */
    int stars_total, rounds_total, sessions;
    uint32_t unlocked;      /* bit g = game g can be played */
    /* the tales: "l'incantesimo grigio", "la notte senza stelle", "la musica perduta" */
    int arc;                /* the adventure being told, 0.. (saved 1-based as "avventura") */
    int chapter;            /* places of it freed so far, 0..5 (5 = that adventure is over) */
    int charge;             /* rounds played towards the next duel */
    uint32_t seen;          /* SEEN_* bits, 8 per adventure: scenes already told */
    int epilogue;           /* the party after the fourth adventure (0.12): 0 not yet, 1 due, 2 told */
    uint64_t stickers;      /* bit i = STICKERS[i] won */
    int reveal;             /* a game unlocked, still to be shown turning around in the menu; -1 */
    /* for the grown-ups: what has been played */
    int rounds[GAME_COUNT];    /* rounds finished, per game */
    int answers[GAME_COUNT];   /* answers given, per game */
    int first_try[GAME_COUNT]; /* ...right at the first try */
    long play_seconds;         /* time spent playing */
    char last_day[12];         /* "2026-09-28": the last day she played */
} progress_t;

typedef struct {
    int session_minutes; /* 0 = no limit */
    int start_level;     /* Conta only; the other games start at 1 */
    int questions_per_round;
    int vol_music, vol_voice, vol_sfx;
    bool rumble;
    bool unlock_gradual;    /* new games unlock as rounds are played */
    int rounds_per_unlock;
    char child_name[16];    /* A-Z: the name of profile 1 and of the personalised voice clips */
    bool story;             /* the tale with the map and the duels */
    int rounds_per_duel;    /* rounds that charge the wand */
    bool animations;        /* transitions between scenes and the living backgrounds (0.9.0) */
} config_t;

void config_defaults(config_t *c);
void config_load(config_t *c, const char *data_dir); /* defaults + <data>/deva_adventures.cfg */
/* Options changed inside the game ("Opzioni") live in the save directory,
 * deva_adventures_opzioni.cfg: only the values that differ from the parent's
 * .cfg file, plus the profile in use. */
void options_load(config_t *c, int *slot, const char *save_dir);
bool options_save(const config_t *c, const config_t *base, int slot, const char *save_dir);
void set_name(char *dst, size_t n, const char *v); /* keeps A-Z, upper case */

void progress_reset(progress_t *p, int start_level);
bool profile_exists(const char *save_dir, int slot);
void profile_log_path(char *out, size_t n, const char *save_dir, int slot);
bool progress_load(progress_t *p, const char *save_dir, int slot, int start_level);
bool progress_save(const progress_t *p, const char *save_dir, int slot);
/* 0.15: a save finishes (flushed to the card) in a helper thread; this waits for the one in flight.
 * Loads, saves and profile_remove call it themselves; the game calls it before it closes. */
void save_wait(void);
bool profile_remove(const char *save_dir, int slot); /* the files become .bak (one copy) */
void progress_wear(progress_t *p, int item); /* marks owned + worn */
bool game_is_unlocked(const progress_t *p, const config_t *c, int game);
int progress_unlock_next(progress_t *p); /* game unlocked now, or -1 */
int progress_next_locked(const progress_t *p); /* the game that opens next, -1 = none (0.14) */

/* The parent log, one row per answer. 0.15: secs, from the moment the question could be answered (< 0:
 * not known); replays, the question asked again on request (B); at, seconds since the game started. */
void log_answer(const char *path, int session, const char *game, int level, const char *subject,
                const char *target, const char *chosen, bool correct, int attempt, double secs, int replays,
                long at);
/* The session diary (0.15): start, scenes, pauses, reminders, end; one row per event. */
void profile_diary_path(char *out, size_t n, const char *save_dir, int slot);
void log_diary(const char *path, int session, long at, const char *event, const char *detail);

#endif
