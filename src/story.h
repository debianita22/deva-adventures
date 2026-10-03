/* Deva's Awesome Adventures - the tales.
 *
 * 1. "L'incantesimo grigio": Strega Grisella has made the Regno delle Stelle
 *    grey, put a spell on four good monsters and given each one a colour gem;
 *    the fifth she keeps. Mago Pistacchio gives Deva a magic wand: every round
 *    played charges it; a charged wand challenges the monster of the next
 *    place on the map, in a duel of questions borrowed from the games. The
 *    spell broken, each monster is good again and gives back its gem, and its
 *    place gets its colours back. The witch, last, turns out to be sad and
 *    lonely: Deva invites her to dance, and the colours come back everywhere.
 * 2. "La notte senza stelle" (from the session after the first one ended):
 *    lo Stregone Mezzanotte has put the stars of the sky in his sack, put a
 *    spell on four good monsters and given each one a big star; the golden one
 *    he keeps. Grisella, good now, helps Deva. Each big star given back goes
 *    back to the sky and lights its place. The wizard, last, is afraid of the
 *    dark (that is why he wanted all the light): Deva gives him a magic
 *    lantern, and he puts all the stars back.
 * 3. "La musica perduta" (0.10.0, from the session after the second one ended):
 *    l'Orco Brontolone blows all the music of the Valle della Musica into five
 *    magic notes (do, re, mi, fa, sol), puts a spell on four good monsters and
 *    gives each one a note; the golden one, sol, he keeps. Lo Stregone
 *    Mezzanotte, good now, is Deva's guide. Each note given back brings the
 *    music back to its place (and plays). The ogre, last, is grumpy because he
 *    cannot sleep: Deva plays a lullaby with the five notes, he falls asleep,
 *    wakes up happy, and gives the music back to the whole valley.
 * 4. "I giocattoli del Re Capriccio" (0.11.0, the session after the third one
 *    ended): in the Regno dei Giocattoli the toys are alive, until the little
 *    Re Capriccio ("e' mio! e' tutto mio!") stops them all and takes the five
 *    wind-up keys; four toy monsters keep four of them under his spell, the
 *    golden one he keeps. The ogre, good now, is Deva's guide. Each key given
 *    back sets the toys of its place moving again. The king, last, cries:
 *    nobody plays with him because he wants everything for himself. Deva
 *    teaches him to take turns and to lend his toys, and the toys are free.
 * 0.12.0: in every ending the kind act is hers (she dances with the witch with the
 * cross, lights the lantern, plays the lullaby note by note, takes turns with the
 * ball); a place under the spell is grey (dark) until its friend is free; and in
 * the session after the fourth adventure, an epilogue: the party for Deva, where
 * each old villain remembers her kind act.
 * SPDX-License-Identifier: MIT
 */
#ifndef DEVA_STORY_H
#define DEVA_STORY_H

#include "common.h"

#define CH_COUNT 5 /* places of an adventure; the last one is the villain's */
#define CH_VILLAIN (CH_COUNT - 1)

enum { ARC_GRIGIO, ARC_NOTTE, ARC_MUSICA, ARC_GIOCHI, ARC_COUNT };

typedef struct {
    const char *place;   /* background "bg_<place>" */
    const char *foe;     /* "ciuffone": sprites mo_<foe>_c/r/b, voices sf_<foe>, vt_<foe>, icons
                            menu_sfida_<foe>, amico_<foe>; the villain's sprites are in arc_t */
    const char *gem;     /* sprite of what it keeps: "gemma_verde", "stella_rossa", "nota_rossa" */
    int games[6];        /* quiz games of the duel, -1 = end: each once before any comes back;
                            the villain's first one (Emozioni) is asked last, on the feeling of
                            the ending, just before "how does the witch feel?" */
} chapter_t;

typedef struct {
    const char *name;               /* for the log: "grigio", "notte" */
    chapter_t ch[CH_COUNT];
    const char *map, *map_off;      /* the map set free, and under the spell */
    const char *anchor;             /* prefix of the map anchors in the atlas: "" or "m2_" */
    int loc[6][2], reg[6][3];       /* the same, when the atlas has none */
    const char *cap;                /* voice "<cap><n>": the next place, told once */
    const char *freed;              /* voice "<freed><n>": a place gets its colours (stars) back */
    const char *map_done;           /* voice: the whole map is free */
    const char *guide;              /* who speaks on the map: "mago", "strega" (the voice itself) */
    const char *music;              /* of the map */
    /* the villain, in the last place */
    const char *v_bad, *v_talk, *v_sad, *v_good, *v_good_talk;
    const char *v_hit;              /* voice when an orb breaks */
} arc_t;

extern const arc_t ARCS[ARC_COUNT];

/* scenes already told, bits of progress_t.seen: 8 bits per adventure (4 adventures: 32 bits), see story_seen() */
enum {
    SEEN_PROLOGO = 1u << 0,
    SEEN_CAP1 = 1u << 1, /* ..5: the intro of each place, CAP1 << ch */
    SEEN_REGOLE = 1u << 6,
    SEEN_FINALE = 1u << 7,
};

/* the tales told between the scenes (each adventure has its prologue and ending; the epilogue,
   0.12, is the party for Deva after the fourth adventure) */
enum { R_PROLOGO, R_SFIDA, R_VITTORIA, R_FINALE, R_EPILOGO, R_COUNT };

/* what the map shows when it opens */
enum { MAP_PLAIN, MAP_FREED, MAP_GO };

int story_arc(void);                 /* the adventure being told, ARC_* */
const arc_t *story_arc_def(void);
const chapter_t *story_ch(int ch);   /* a place of the current adventure */
bool story_villain(int ch);          /* the last place: the witch, the wizard */
const char *story_foe_sprite(int ch, char form); /* 'c' spell, 'r' talking, 'b' good */
bool story_seen(uint32_t bit);       /* SEEN_* of the current adventure */
void story_mark_seen(uint32_t bit);
bool story_arc_done(int arc);        /* that adventure has been told to the end */
const char *story_line(const char *base); /* "g_sfida" -> "g_sfida_grisella" when the next duel is
                                             with the witch (the wizard) and that line exists */
void story_session_start(void);      /* the next adventure begins the session after one ended */
bool story_epilogue_due(void);       /* the fourth adventure ended in an earlier session: the party for Deva */

bool story_active(void);     /* the tale is on (config) and the current adventure is not over */
int story_chapter(void);     /* the next place to free, 0..4; CH_COUNT when all are free */
int story_charge_needed(void);
bool story_charged(void);
void story_round_done(void); /* a round of any game charges the wand */

void racconto_play(int id, int next_scene); /* tell a tale, then go to that scene */
void racconto_replay(int arc, int next_scene); /* the prologue and the ending of an adventure already told,
                                                  again (from the album): nothing of the progress changes */
bool racconto_replaying(void);                 /* such a replay is on (the story pretends to be in that adventure) */
void racconto_replay_cancel(void);             /* the scene changed: the replay is over */
void mappa_open(int event);                 /* the map, with an event to show */
void sfida_start(void);                     /* the duel of the current chapter */

#endif
