/* Deva's Awesome Adventures - the "pick one of three cards" engine shared by
 * Conta, Parole, Sequenze, Il mio nome and the other card games.
 *
 * The engine owns the flow (prepare, spoken question, choice, gentle errors,
 * guided help after two mistakes, stars, adaptive level, end of round); each
 * game describes itself with a quiz_def_t of callbacks. A question may have
 * several steps (the letters of a word, the frames of a story): each step is
 * one choice, the star comes when the last step is right.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_QUIZ_H
#define DEVA_QUIZ_H

#include "common.h"

enum { QUIZ_CARD_NUMBER, QUIZ_CARD_PICTURE }; /* 56x48 number cards, 52x52 picture cards */
enum { CARD_NORMAL, CARD_SELECTED, CARD_OFF };
enum { Q_INTRO, Q_PREPARE, Q_INPUT, Q_RIGHT, Q_WRONG, Q_HINT, Q_STEP };

typedef struct quiz quiz_t;

typedef struct {
    const char *name;  /* log / test name */
    int game;          /* GAME_* */
    const char *bg;    /* background image */
    const char *intro; /* voice the first time the game is played in a session */
    const char *(*intro_voice)(void); /* optional: that voice, when it depends on the moment */
    int card;          /* QUIZ_CARD_*, default for every question (setup may change q->card) */
    int max_questions; /* questions per round at most (0 = the parent's setting) */
    bool gentle;       /* a moment of the tale, not a quiz: a right answer gets no "evviva" */
    /* new question at this level: fill correct (and steps) + whatever the game needs */
    void (*setup)(quiz_t *q, int level);
    /* per-frame pre-question animation; return true when it is over */
    bool (*prepare)(quiz_t *q);
    void (*tick)(quiz_t *q);           /* optional: called every frame, in every state */
    void (*ask)(quiz_t *q);            /* speak the question (of the current step) */
    void (*hover)(quiz_t *q, int i);   /* card i highlighted */
    /* optional: the cross while she chooses; true = handled (e.g. the arrow of a
       step picks the card with that arrow), false = the usual left/right */
    bool (*key)(quiz_t *q);
    void (*right)(quiz_t *q);          /* optional: extra reinforcement, queued after "bravissima" */
    void (*next_step)(quiz_t *q);      /* multi-step questions: step done, set up the next one */
    /* guided help, called whenever the voice is idle; return true when done */
    bool (*hint)(quiz_t *q);
    void (*draw)(quiz_t *q);           /* question content inside the panel */
    void (*intro_draw)(quiz_t *q);     /* optional: the panel during the intro (default: empty stage) */
    void (*card_draw)(quiz_t *q, int i, int cx, int cy, int style);
    void (*card_value)(quiz_t *q, int i, char *buf, size_t n); /* for the parent log */
    const char *(*subject)(quiz_t *q);                         /* for the parent log */
} quiz_def_t;

struct quiz {
    const quiz_def_t *def;
    int state, t;
    int card; /* QUIZ_CARD_* of the current question */
    int level;
    int correct, sel;
    bool off[3];
    int attempts;    /* mistakes in this step */
    int errors;      /* mistakes in the whole question */
    bool assisted;
    int step, steps; /* multi-step questions; steps = 1 otherwise */
    int question, stars;
    int idle;
    int hint_step, hint_wait;
    int panel_cx, panel_cy;
    bool duel;       /* asked in a duel of the tale (quiz_enter_host) */
    bool gentle;     /* this question: a sad or scary feeling, confirmed softly (setup sets it) */
    int deal;        /* frames since the cards were dealt (they fly in from below) */
    int wrong;       /* the card of the last mistake (it shakes its head) */
    bool listen;     /* a new question is being asked: the red button waits (0.14) */
};

/* A host scene runs the engine with questions borrowed from several games
 * (the duels of the tale): it picks the game of every question, draws the
 * scenery, reacts to the answers and decides what happens at the end. */
typedef struct {
    const quiz_def_t *(*next)(int question); /* the game of this question */
    int questions;                            /* how many questions */
    void (*draw_back)(quiz_t *q);             /* background and whatever stands behind the panel */
    void (*draw_front)(quiz_t *q);            /* effects over everything */
    void (*right)(quiz_t *q);                 /* after the engine's praise */
    void (*wrong)(quiz_t *q);
    void (*tick)(quiz_t *q);
    void (*done)(void);                       /* the last question is over */
} quiz_host_t;

void quiz_enter(const quiz_def_t *def);
void quiz_enter_host(const quiz_host_t *host);
void quiz_update(void);
void quiz_draw(void);

/* the card games, for the duels of the tale */
extern const quiz_def_t QUIZ_CONTA, QUIZ_PAROLE, QUIZ_SEQUENZE, QUIZ_NOME, QUIZ_EMOZIONI, QUIZ_STORIE,
    QUIZ_TRUCCO, QUIZ_FORME, QUIZ_SALTI;
/* duel questions of the games that are not card games (scene_duelli.c) */
extern const quiz_def_t QUIZ_PASSI, QUIZ_TAMBURO, QUIZ_DOVE, QUIZ_SPARITO;
/* the games of 0.10.0 (their own scenes; "Il sentiero" asks QUIZ_STRADA in the duels) */
extern const quiz_def_t QUIZ_LETTERE, QUIZ_OMBRE, QUIZ_STRADA;
extern const quiz_def_t QUIZ_NEGOZIO, QUIZ_MISURE;

int quiz_card_cx(int i);
int quiz_card_cy(void);
void quiz_select(int i);  /* highlight card i (sound, voice, log), if it is still on */
int quiz_round_len(void); /* questions in this round */
void emozioni_force(const char *what); /* the next Emozioni question: "faccia:triste", "storia:luna" */

#endif
