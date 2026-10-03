/* Deva's Awesome Adventures - the tales: adventures and chapters, the wand's
 * charge, and the "racconti", the little illustrated scenes told between the
 * games.
 *
 * A racconto is a list of steps: a background, the characters on stage (a
 * CAST_* layout), a line of voice, a sound, an effect. Each step lasts as
 * long as its voice plus a pause, so a child who cannot read just watches;
 * the red button skips ahead. Chapter-dependent parts ("@") take the monster
 * and the place of the chapter being told. Each adventure has its own
 * prologue and ending; the duel's opening and the victory are shared.
 * SPDX-License-Identifier: MIT
 */
#include "story.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "amb.h"
#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "trans.h"

/* the games of each duel (the ones she can already play): the newer ones make
   the monster jump on a trampoline, dress it up, bring shapes to the spell; in
   the second adventure every game has its duel question */
const arc_t ARCS[ARC_COUNT] = {
    {"grigio",
     {{"bosco", "ciuffone", "gemma_verde", {GAME_CONTA, GAME_SEQUENZE, GAME_GINNASTICA, -1}},
      {"palude", "melmoso", "gemma_azzurra", {GAME_PAROLE, GAME_CONTA, GAME_TRUCCO, -1}},
      {"grotta", "rocciolo", "gemma_rosa", {GAME_SEQUENZE, GAME_STORIE, GAME_CONTA, GAME_FORME, -1}},
      {"nuvole", "tuonello", "gemma_gialla", {GAME_EMOZIONI, GAME_PAROLE, GAME_CONTA, GAME_GINNASTICA, GAME_TRUCCO, -1}},
      {"castello", "grisella", "gemma_viola",
       {GAME_EMOZIONI, GAME_CONTA, GAME_PAROLE, GAME_SEQUENZE, GAME_FORME, GAME_TRUCCO}}},
     "bg_mappa", "bg_mappa_grigia", "",
     {{44, 214}, {106, 176}, {170, 214}, {242, 176}, {186, 118}, {270, 90}},
     {{0, 0, 0}, {100, 150, 52}, {170, 204, 50}, {244, 150, 54}, {180, 76, 58}, {272, 50, 60}},
     "st_cap", "vt_colori_", "mappa_finita", "mago", "mappa",
     "strega_c", "strega_cp", "strega_pianto", "strega_b", "strega_bp", "sf_colpo_strega"},
    {"notte",
     {{"spiaggia", "polpone", "stella_rossa", {GAME_CONTA, GAME_DOVE, GAME_MEMORY, -1}},
      {"giardino", "lumacone", "stella_arancione", {GAME_PAROLE, GAME_FORME, GAME_BALLA, -1}},
      {"ghiaccio", "nevone", "stella_azzurra", {GAME_SEQUENZE, GAME_RITMO, GAME_NOME, -1}},
      {"vulcano", "fumino", "stella_verde", {GAME_GINNASTICA, GAME_TRUCCO, GAME_STORIE, -1}},
      {"torre", "mezzanotte", "stella_oro",
       {GAME_EMOZIONI, GAME_DOVE, GAME_BALLA, GAME_RITMO, GAME_MEMORY, GAME_CONTA}}},
     "bg_mappa2", "bg_mappa2_buia", "m2_",
     {{30, 214}, {152, 214}, {98, 170}, {42, 120}, {160, 110}, {270, 88}},
     {{0, 0, 0}, {164, 206, 54}, {98, 150, 48}, {44, 84, 54}, {162, 76, 50}, {272, 56, 60}},
     "st2_cap", "vt2_stelle_", "mappa2_finita", "strega", "notte",
     "stregone_c", "stregone_cp", "stregone_paura", "stregone_b", "stregone_bp", "sf_colpo_stregone"},
    /* the third: the games of 0.10 come to the duels, with the musical ones */
    {"musica",
     {{"dolci", "caramellone", "nota_rossa", {GAME_CONTA, GAME_LETTERE, GAME_BALLA, -1}},
      {"funghi", "fungone", "nota_arancione", {GAME_OMBRE, GAME_SEQUENZE, GAME_RITMO, -1}},
      {"lago", "ranocchione", "nota_azzurra", {GAME_SENTIERO, GAME_PAROLE, GAME_MEMORY, -1}},
      {"circo", "trombone", "nota_verde", {GAME_GINNASTICA, GAME_FORME, GAME_DOVE, GAME_LETTERE, -1}},
      {"casa_orco", "orco", "nota_oro",
       {GAME_EMOZIONI, GAME_OMBRE, GAME_SENTIERO, GAME_RITMO, GAME_NOME, GAME_TRUCCO}}},
     "bg_mappa3", "bg_mappa3_zitta", "m3_",
     {{28, 214}, {150, 216}, {58, 154}, {176, 152}, {96, 96}, {262, 94}},
     {{0, 0, 0}, {152, 198, 52}, {62, 130, 50}, {212, 140, 54}, {98, 70, 52}, {268, 62, 60}},
     "st3_cap", "vt3_musica_", "mappa3_finita", "stregone", "valle",
     "orco_c", "orco_cp", "orco_brontola", "orco_b", "orco_bp", "sf_colpo_orco"},
    /* the fourth: the toys stand still; the games of 0.11 (the shop, the measures) come to the duels */
    {"giocattoli",
     {{"fabbrica", "robottone", "chiave_rossa", {GAME_NEGOZIO, GAME_CONTA, GAME_SENTIERO, -1}},
      {"birilli", "saltamolla", "chiave_arancione", {GAME_MISURE, GAME_RITMO, GAME_PAROLE, -1}},
      {"cubi", "dinozzo", "chiave_azzurra", {GAME_FORME, GAME_MEMORY, GAME_LETTERE, -1}},
      {"giostra", "trottolina", "chiave_verde", {GAME_BALLA, GAME_OMBRE, GAME_NEGOZIO, GAME_DOVE, -1}},
      {"castello_giochi", "re", "chiave_oro",
       {GAME_EMOZIONI, GAME_MISURE, GAME_NEGOZIO, GAME_SEQUENZE, GAME_STORIE, GAME_TRUCCO}}},
     "bg_mappa4", "bg_mappa4_ferma", "m4_",
     {{28, 214}, {150, 214}, {262, 160}, {146, 140}, {52, 96}, {226, 86}},
     {{0, 0, 0}, {150, 196, 52}, {262, 140, 52}, {146, 120, 50}, {56, 74, 52}, {234, 58, 60}},
     "st4_cap", "vt4_carica_", "mappa4_finita", "orco", "giocattoli",
     "re_c", "re_cp", "re_piange", "re_b", "re_bp", "sf_colpo_re"},
};

static int s_replay = -1; /* an adventure told again from the album (racconto_replay), -1 = none */

int story_arc(void) { return clampi(s_replay >= 0 ? s_replay : G.prog.arc, 0, ARC_COUNT - 1); }
bool racconto_replaying(void) { return s_replay >= 0; }
void racconto_replay_cancel(void) { s_replay = -1; }
const arc_t *story_arc_def(void) { return &ARCS[story_arc()]; }
const chapter_t *story_ch(int ch) { return &ARCS[story_arc()].ch[clampi(ch, 0, CH_COUNT - 1)]; }
bool story_villain(int ch) { return ch == CH_VILLAIN; }

const char *story_foe_sprite(int ch, char form)
{
    static char name[40];
    const arc_t *a = story_arc_def();
    if (story_villain(ch))
        return form == 'b' ? a->v_good : (form == 'r' ? a->v_talk : a->v_bad);
    snprintf(name, sizeof(name), "mo_%s_%c", story_ch(ch)->foe, form);
    return name;
}

static uint32_t arc_bit(int arc, uint32_t bit) { return bit << (8 * clampi(arc, 0, 3)); }
bool story_seen(uint32_t bit) { return (G.prog.seen & arc_bit(story_arc(), bit)) != 0; }
void story_mark_seen(uint32_t bit) { G.prog.seen |= arc_bit(story_arc(), bit); }

bool story_arc_done(int arc)
{
    if (arc < story_arc())
        return true;
    return arc == story_arc() && G.prog.chapter >= CH_COUNT && (G.prog.seen & arc_bit(arc, SEEN_FINALE));
}

const char *story_line(const char *base)
{
    /* the witch and the wizard are not monsters: "sfida la strega!"; the toys of
       the fourth adventure are toys under a spell: "sfida il giocattolo!" */
    static char id[48];
    int ch = story_chapter();
    if (!story_active())
        return base;
    if (story_villain(ch))
        snprintf(id, sizeof(id), "%s_%s", base, story_ch(ch)->foe);
    else if (story_arc() == ARC_GIOCHI)
        snprintf(id, sizeof(id), "%s_giocattolo", base);
    else
        return base;
    return snd_exists(SND_VOICE, id) ? id : base;
}

void story_session_start(void)
{
    /* the ending of an adventure closes the session ("le avventure di Deva
       continuano..."): the next one begins the next time she plays; after the
       last one, the party for Deva ("e la prossima volta... una grande festa!") */
    if (story_arc_done(ARC_COUNT - 1) && G.prog.epilogue == 0) {
        G.prog.epilogue = 1;
        BOT("story epilogue due\n");
        game_save();
        return;
    }
    if (!story_arc_done(story_arc()) || story_arc() + 1 >= ARC_COUNT)
        return;
    G.prog.arc = story_arc() + 1;
    G.prog.chapter = 0;
    G.prog.charge = 0;
    BOT("story arc=%d %s\n", G.prog.arc + 1, ARCS[G.prog.arc].name);
    game_save();
}

bool story_epilogue_due(void) { return G.cfg.story && G.prog.epilogue == 1 && story_arc_done(ARC_COUNT - 1); }

bool story_active(void) { return G.cfg.story && G.prog.chapter < CH_COUNT; }
int story_chapter(void) { return clampi(G.prog.chapter, 0, CH_COUNT); }
int story_charge_needed(void) { return clampi(G.cfg.rounds_per_duel, 1, 5); }
bool story_charged(void) { return story_active() && G.prog.charge >= story_charge_needed(); }

void story_round_done(void)
{
    if (!story_active() || G.prog.charge >= story_charge_needed())
        return;
    G.prog.charge++;
    G.just_charged = true;
    BOT("story charge=%d/%d chapter=%d\n", G.prog.charge, story_charge_needed(), G.prog.chapter);
}

/* ------------------------------------------------------------------ racconti */

enum {
    CAST_NONE, CAST_FLY, CAST_WITCH_MAP, CAST_GEMS, CAST_MAGO, CAST_WAND, CAST_FOE, CAST_FOE_MAGO,
    CAST_FREE, CAST_GEM, CAST_CRY, CAST_HUG, CAST_WITCH_GOOD, CAST_PARTY,
    /* the second adventure */
    CAST_STEAL, CAST_WIZ_MAP, CAST_STARS, CAST_GRISELLA, CAST_FRIENDS, CAST_SCARED, CAST_LANTERN,
    CAST_WIZ_GOOD, CAST_SKY, CAST_PARTY2,
    /* the third adventure */
    CAST_VALLEY, CAST_OGRE_HILL, CAST_OGRE_BLOW, CAST_NOTES, CAST_MEZZANOTTE, CAST_FRIENDS3,
    CAST_OGRE_GRUMPY, CAST_OGRE_TIRED, CAST_LULLABY, CAST_OGRE_SLEEP, CAST_OGRE_GOOD, CAST_PARTY3,
    /* the fourth adventure */
    CAST_TOYLAND, CAST_KING_HOME, CAST_KING_SPELL, CAST_KEYS, CAST_ORCO_FRIEND, CAST_FRIENDS4,
    CAST_KING_CRY, CAST_SHARE, CAST_KING_GOOD, CAST_TOYS_BACK, CAST_PARTY4,
    /* 0.12: the treasures are born before they are given away (the step "Tutti i colori finirono in
       cinque gemme magiche"), the kind acts done by her, the epilogue */
    CAST_GEMS_BORN, CAST_STARS_BORN, CAST_NOTES_BORN, CAST_KEYS_BORN,
    CAST_DANCE, CAST_SECRET, CAST_NOTES_READY, CAST_BALL_READY,
    CAST_EP_INVITE, CAST_EP_FRIEND, CAST_EP_MAGO, CAST_EP_PARTY
};
enum { FX_NONE, FX_LIGHTNING, FX_SHAKE, FX_GREY, FX_POOF, FX_TRANSFORM, FX_CONFETTI, FX_SPARKLE, FX_LIGHT };
enum { SP_NONE, SP_MAGO, SP_STREGA, SP_FOE, SP_WIZ, SP_ORCO, SP_RE };
enum { IF_ALWAYS, IF_RULES }; /* a step told only sometimes */
/* the kind act of an ending, done by her (0.12): the step waits for it */
enum { ACT_NONE, ACT_DANCE, ACT_LANTERN, ACT_LULLABY, ACT_BALL, ACT_FIREWORKS };
/* how the place of the chapter looks: as it is, under the spell (grey, dark), or the colours coming
   back from the friend set free (0.12) */
enum { LOOK_PLAIN, LOOK_SPELL, LOOK_REVEAL };

typedef struct {
    const char *bg;    /* background image, "@" = the place of the chapter, NULL = keep */
    const char *voice; /* "@sf", "@vt": the chapter's foe */
    const char *sfx;   /* "@" = the foe's own sound (growl, thunder for the villain) */
    const char *music; /* NULL = keep, "-" = silence */
    uint8_t cast, fx, speaker, cond;
    short hold;        /* frames after the voice */
    uint8_t act, look;
} rstep_t;

static const rstep_t PROLOGO[] = {
    {"bg_mappa", "st_prologo_1", "sparkle", "mappa", CAST_NONE, FX_SPARKLE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_2", "tuono", "sfida", CAST_FLY, FX_LIGHTNING, SP_NONE, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_3", NULL, NULL, CAST_WITCH_MAP, FX_GREY, SP_STREGA, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_mappa_grigia", "st_prologo_4", NULL, NULL, CAST_GEMS_BORN, FX_NONE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_4b", NULL, NULL, CAST_GEMS, FX_NONE, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_5", "poof", "mappa", CAST_MAGO, FX_POOF, SP_MAGO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_6", "incantesimo", NULL, CAST_WAND, FX_NONE, SP_MAGO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st_prologo_7", NULL, NULL, CAST_MAGO, FX_NONE, SP_MAGO, 0, 30, ACT_NONE, LOOK_PLAIN},
};
/* the duel's opening and the victory: the place is under the spell until the friend is free (0.12) */
static const rstep_t SFIDA[] = {
    {"@", "@sf", "@", "sfida", CAST_FOE, FX_SHAKE, SP_FOE, 0, 20, ACT_NONE, LOOK_SPELL},
    {NULL, "@regole", "poof", NULL, CAST_FOE_MAGO, FX_POOF, SP_MAGO, IF_RULES, 10, ACT_NONE, LOOK_SPELL}, /* by the guide */
};
static const rstep_t VITTORIA[] = {
    {"@", "vt_rompe", "trasforma", "mappa", CAST_FREE, FX_TRANSFORM, SP_NONE, 0, 40, ACT_NONE, LOOK_REVEAL},
    {NULL, "@vt", "fanfare", NULL, CAST_GEM, FX_NONE, SP_FOE, 0, 30, ACT_NONE, LOOK_PLAIN},
};
/* the endings: the villain's place under the spell until the villain is good again; the kind act is hers */
static const rstep_t FINALE[] = {
    {"bg_castello", "fn_triste", NULL, "nanna", CAST_CRY, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "fn_strega_1", NULL, NULL, CAST_CRY, FX_NONE, SP_STREGA, 0, 20, ACT_NONE, LOOK_SPELL},
    {NULL, "fn_invito", "sparkle", NULL, CAST_HUG, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "fn_balla", NULL, "palco", CAST_DANCE, FX_NONE, SP_NONE, 0, 40, ACT_DANCE, LOOK_SPELL},
    {NULL, "fn_strega_2", "trasforma", "palco", CAST_WITCH_GOOD, FX_TRANSFORM, SP_STREGA, 0, 30, ACT_NONE, LOOK_REVEAL},
    {"bg_mappa_grigia", "fn_fine_1", "sparkle", "mappa", CAST_SKY, FX_LIGHT, SP_NONE, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_palco", "fn_fine_2", "fanfare", "palco", CAST_PARTY, FX_CONFETTI, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "fn_fine_3", NULL, NULL, CAST_PARTY, FX_CONFETTI, SP_NONE, 0, 150, ACT_NONE, LOOK_PLAIN},
};
static const rstep_t PROLOGO2[] = {
    {"bg_mappa2", "st2_prologo_1", "sparkle", "nanna", CAST_NONE, FX_SPARKLE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_2", "tuono", "sfida", CAST_STEAL, FX_LIGHTNING, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {"bg_mappa2_buia", "st2_prologo_3", NULL, NULL, CAST_WIZ_MAP, FX_NONE, SP_WIZ, 0, 40, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_4", NULL, NULL, CAST_STARS_BORN, FX_NONE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_4b", NULL, NULL, CAST_STARS, FX_NONE, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_5", "poof", "mappa", CAST_GRISELLA, FX_POOF, SP_STREGA, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_6", "incantesimo", NULL, CAST_FRIENDS, FX_NONE, SP_MAGO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_prologo_7", NULL, NULL, CAST_FRIENDS, FX_NONE, SP_STREGA, 0, 30, ACT_NONE, LOOK_PLAIN},
};
/* in the dark tower, the light of her lantern brings the colours back */
static const rstep_t FINALE2[] = {
    {"bg_torre", "st2_spaventato", NULL, "nanna", CAST_SCARED, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "st2_stregone_1", NULL, NULL, CAST_SCARED, FX_NONE, SP_WIZ, 0, 20, ACT_NONE, LOOK_SPELL},
    {NULL, "st2_lanterna", NULL, NULL, CAST_LANTERN, FX_NONE, SP_NONE, 0, 40, ACT_LANTERN, LOOK_SPELL},
    {NULL, "st2_segreto", "sparkle", NULL, CAST_SECRET, FX_NONE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_stregone_2", "trasforma", "palco", CAST_WIZ_GOOD, FX_TRANSFORM, SP_WIZ, 0, 30, ACT_NONE, LOOK_PLAIN},
    {"bg_mappa2_buia", "st2_fine_1", "sparkle", "mappa", CAST_SKY, FX_LIGHT, SP_NONE, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_festa2", "st2_fine_2", "fanfare", "palco", CAST_PARTY2, FX_CONFETTI, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st2_fine_3", NULL, NULL, CAST_PARTY2, FX_CONFETTI, SP_NONE, 0, 150, ACT_NONE, LOOK_PLAIN},
};

/* the third adventure: the ogre blows the music away; at the end she plays him a lullaby */
static const rstep_t PROLOGO3[] = {
    {"bg_mappa3", "st3_prologo_1", "sparkle", "valle", CAST_VALLEY, FX_SPARKLE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_2", "pestone", "sfida", CAST_OGRE_HILL, FX_SHAKE, SP_NONE, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_3", "whoosh", NULL, CAST_OGRE_BLOW, FX_GREY, SP_ORCO, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_mappa3_zitta", "st3_prologo_4", NULL, NULL, CAST_NOTES_BORN, FX_NONE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_4b", NULL, NULL, CAST_NOTES, FX_NONE, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_5", "poof", "valle", CAST_MEZZANOTTE, FX_POOF, SP_WIZ, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_6", "incantesimo", NULL, CAST_FRIENDS3, FX_NONE, SP_MAGO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_prologo_7", NULL, NULL, CAST_FRIENDS3, FX_NONE, SP_WIZ, 0, 30, ACT_NONE, LOOK_PLAIN},
};
static const rstep_t FINALE3[] = {
    {"bg_casa_orco", "st3_arrabbiato", NULL, "nanna", CAST_OGRE_GRUMPY, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "st3_orco_1", "yawn", NULL, CAST_OGRE_TIRED, FX_NONE, SP_ORCO, 0, 20, ACT_NONE, LOOK_SPELL},
    {NULL, "st3_ninna", "sparkle", "-", CAST_NOTES_READY, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "st3_suona", NULL, NULL, CAST_LULLABY, FX_NONE, SP_NONE, 0, 50, ACT_LULLABY, LOOK_SPELL},
    {NULL, "st3_dorme", NULL, NULL, CAST_OGRE_SLEEP, FX_NONE, SP_NONE, 0, 60, ACT_NONE, LOOK_SPELL},
    {NULL, "st3_orco_2", "trasforma", "palco", CAST_OGRE_GOOD, FX_TRANSFORM, SP_ORCO, 0, 30, ACT_NONE, LOOK_REVEAL},
    {"bg_mappa3_zitta", "st3_fine_1", "sparkle", "valle", CAST_SKY, FX_LIGHT, SP_NONE, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_circo", "st3_fine_2", "fanfare", "palco", CAST_PARTY3, FX_CONFETTI, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st3_fine_3", NULL, NULL, CAST_PARTY3, FX_CONFETTI, SP_NONE, 0, 150, ACT_NONE, LOOK_PLAIN},
};

/* the fourth adventure: the toys stand still; at the end Deva and the king play taking turns */
static const rstep_t PROLOGO4[] = {
    {"bg_mappa4", "st4_prologo_1", "sparkle", "giocattoli", CAST_TOYLAND, FX_SPARKLE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_2", "trombetta", "sfida", CAST_KING_HOME, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_3", "incantesimo", NULL, CAST_KING_SPELL, FX_GREY, SP_RE, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_mappa4_ferma", "st4_prologo_4", NULL, NULL, CAST_KEYS_BORN, FX_NONE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_4b", NULL, NULL, CAST_KEYS, FX_NONE, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_5", "poof", "giocattoli", CAST_ORCO_FRIEND, FX_POOF, SP_ORCO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_6", "incantesimo", NULL, CAST_FRIENDS4, FX_NONE, SP_MAGO, 0, 10, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_prologo_7", NULL, NULL, CAST_FRIENDS4, FX_NONE, SP_ORCO, 0, 30, ACT_NONE, LOOK_PLAIN},
};
static const rstep_t FINALE4[] = {
    {"bg_castello_giochi", "st4_triste", NULL, "nanna", CAST_KING_CRY, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "st4_re_1", NULL, NULL, CAST_KING_CRY, FX_NONE, SP_RE, 0, 20, ACT_NONE, LOOK_SPELL},
    {NULL, "st4_turno", "sparkle", NULL, CAST_BALL_READY, FX_NONE, SP_NONE, 0, 10, ACT_NONE, LOOK_SPELL},
    {NULL, "st4_lancia", NULL, NULL, CAST_SHARE, FX_NONE, SP_NONE, 0, 40, ACT_BALL, LOOK_SPELL},
    {NULL, "st4_re_2", "trasforma", "palco", CAST_KING_GOOD, FX_TRANSFORM, SP_RE, 0, 30, ACT_NONE, LOOK_REVEAL},
    {"bg_mappa4_ferma", "st4_fine_1", "carica", "giocattoli", CAST_TOYS_BACK, FX_LIGHT, SP_NONE, 0, 40, ACT_NONE, LOOK_PLAIN},
    {"bg_giostra", "st4_fine_2", "fanfare", "palco", CAST_PARTY4, FX_CONFETTI, SP_NONE, 0, 30, ACT_NONE, LOOK_PLAIN},
    {NULL, "st4_fine_3", NULL, NULL, CAST_PARTY4, FX_CONFETTI, SP_NONE, 0, 150, ACT_NONE, LOOK_PLAIN},
};

/* the epilogue (0.12): the session after the fourth adventure, the party for Deva - each old villain
   remembers her kind act, the wizard calls her a real little witch, and she lights the fireworks */
static const rstep_t EPILOGO[] = {
    {"bg_palco", "ep_1", "sparkle", "palco", CAST_EP_INVITE, FX_SPARKLE, SP_NONE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "ep_grisella", "poof", NULL, CAST_EP_FRIEND, FX_POOF, SP_STREGA, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "ep_mezzanotte", "poof", NULL, CAST_EP_FRIEND, FX_POOF, SP_WIZ, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "ep_orco", "poof", NULL, CAST_EP_FRIEND, FX_POOF, SP_ORCO, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "ep_re", "poof", NULL, CAST_EP_FRIEND, FX_POOF, SP_RE, 0, 20, ACT_NONE, LOOK_PLAIN},
    {NULL, "ep_mago", "incantesimo", NULL, CAST_EP_MAGO, FX_TRANSFORM, SP_MAGO, 0, 30, ACT_NONE, LOOK_PLAIN},
    {"bg_notte", "ep_fuochi", "fanfare", NULL, CAST_EP_PARTY, FX_NONE, SP_NONE, 0, 40, ACT_FIREWORKS, LOOK_PLAIN},
    {NULL, "ep_fine", NULL, NULL, CAST_EP_PARTY, FX_CONFETTI, SP_NONE, 0, 150, ACT_NONE, LOOK_PLAIN},
};

typedef struct {
    const rstep_t *steps;
    int n;
    const char *name;
} tale_t;

#define EPILOGUE {EPILOGO, ARRAY_LEN(EPILOGO), "epilogo"}
static const tale_t TALES[ARC_COUNT][R_COUNT] = {
    {{PROLOGO, ARRAY_LEN(PROLOGO), "prologo"},
     {SFIDA, ARRAY_LEN(SFIDA), "sfida"},
     {VITTORIA, ARRAY_LEN(VITTORIA), "vittoria"},
     {FINALE, ARRAY_LEN(FINALE), "finale"},
     EPILOGUE},
    {{PROLOGO2, ARRAY_LEN(PROLOGO2), "prologo2"},
     {SFIDA, ARRAY_LEN(SFIDA), "sfida"},
     {VITTORIA, ARRAY_LEN(VITTORIA), "vittoria"},
     {FINALE2, ARRAY_LEN(FINALE2), "finale2"},
     EPILOGUE},
    {{PROLOGO3, ARRAY_LEN(PROLOGO3), "prologo3"},
     {SFIDA, ARRAY_LEN(SFIDA), "sfida"},
     {VITTORIA, ARRAY_LEN(VITTORIA), "vittoria"},
     {FINALE3, ARRAY_LEN(FINALE3), "finale3"},
     EPILOGUE},
    {{PROLOGO4, ARRAY_LEN(PROLOGO4), "prologo4"},
     {SFIDA, ARRAY_LEN(SFIDA), "sfida"},
     {VITTORIA, ARRAY_LEN(VITTORIA), "vittoria"},
     {FINALE4, ARRAY_LEN(FINALE4), "finale4"},
     EPILOGUE},
};

#define MAX_FLY 6 /* notes of the lullaby, fireworks: in the air at the same time */
static struct {
    int id, next, ch;
    const tale_t *tale;
    int step, t, after; /* frames in the step, frames since the voice ended */
    const char *bg;
    hero_t deva;
    int shake;
    uint32_t train_frame; /* the toy train of the fourth map runs on G.frame, until the spell stops it */
    int hold_skip;        /* frames L + R have been held: the grown-ups skip a line; -1 = wait for release */
    bool fast;            /* ...and while it stays held, the next steps go by too */
    bool wand_given;      /* the prologue: the wand is in her hand from now on */
    /* the kind act of the ending (0.12) */
    bool act_ready;       /* the prompt has been said: her turn */
    int act_n;            /* actions done */
    int act_t;            /* frames since the last action */
    int act_quiet;        /* frames of waiting for her */
    int act_reminds;      /* reminders said; after them it goes on by itself */
    bool act_auto;
    int act_done_t;       /* frames since the act was completed, -1 = not yet */
    int move, move_flip, copy_t; /* the dance: her last move, and the witch copying it */
    float light_r;        /* the lantern: the light around her wand */
    int light_target;
    int lantern_t;        /* the lantern flies to the wizard; -1 = not yet */
    int fly_t[MAX_FLY], fly_k[MAX_FLY], fly_x[MAX_FLY], fly_y[MAX_FLY]; /* notes, fireworks: -1 = free */
    int ball, ball_t;     /* the game of turns: where the ball is, frames in that state */
    int king_talk;        /* frames the king keeps talking ("tocca a me!") */
    int wait_t;           /* frames since "aspetta, tocca a me!" */
    int reveal_t;         /* LOOK_REVEAL: frames of the colours coming back */
} R;

static void act_reset(void);

void racconto_play(int id, int next_scene)
{
    memset(&R, 0, sizeof(R));
    act_reset();
    R.id = id;
    R.next = next_scene;
    R.tale = &TALES[story_arc()][id];
    /* the tale after a duel is about the place just freed */
    R.ch = (id == R_VITTORIA || id == R_FINALE) ? clampi(G.prog.chapter - 1, 0, CH_COUNT - 1)
                                                : clampi(story_chapter(), 0, CH_COUNT - 1);
    game_goto(SC_RACCONTO);
}

/* the prologue and the ending of an adventure already told, one after the other: the story pretends
   to be in that adventure (story_arc) until the ending is over or the scene changes (game_goto) */
void racconto_replay(int arc, int next_scene)
{
    memset(&R, 0, sizeof(R));
    act_reset();
    s_replay = clampi(arc, 0, ARC_COUNT - 1);
    R.id = R_PROLOGO;
    R.next = next_scene;
    R.tale = &TALES[s_replay][R_PROLOGO];
    R.ch = 0;
    BOT("racconto replay arc=%d\n", s_replay + 1);
    game_goto(SC_RACCONTO);
}

static const rstep_t *cur(void) { return &R.tale->steps[R.step]; }

static void map_points(int x[6], int y[6]);

/* the treasures of a prologue, in two steps (0.12): "Tutti i colori finirono in cinque gemme
   magiche" - the villain flies home and the five treasures pop out by it, 8 frames apart - then "La
   strega fece una magia a quattro mostri buoni, e gli diede le gemme da nascondere. La Gemma Viola
   la tenne lei!": the spell, the gems fly to the monsters, the last one stays. The moments of those
   lines, measured on the voice clips (tools/voice/cues.py), in frames: */
static const int BORN_CUES[ARC_COUNT][2] = {{50, 82}, {80, 200}, {1, 76}, {1, 44}};  /* home, "cinque" */
static const int GIVE_CUES[ARC_COUNT][3] = {{60, 162, 260}, {69, 178, 278}, {50, 160, 256}, {39, 139, 240}};
/* ...spell ("una magia"), give ("e gli diede"), own ("La Gemma Viola") */

static bool step_wanted(const rstep_t *s)
{
    if (s->cond == IF_RULES)
        return !story_seen(SEEN_REGOLE);
    return true;
}

static void resolve(const char *id, char *out, size_t n)
{
    const chapter_t *c = story_ch(R.ch);
    if (!strcmp(id, "@sf"))
        snprintf(out, n, "sf_%s", c->foe);
    else if (!strcmp(id, "@vt"))
        snprintf(out, n, "vt_%s", c->foe);
    else if (!strcmp(id, "@regole")) /* the guide of this adventure: the wizard, Grisella, Mezzanotte */
        snprintf(out, n, "%s", story_arc() == ARC_GIOCHI   ? "sf_regole4"
                               : story_arc() == ARC_MUSICA ? "sf_regole3"
                               : (story_arc() == ARC_NOTTE ? "sf_regole2" : "sf_regole"));
    else
        snprintf(out, n, "%s", id);
}

/* ---- the kind acts of the endings (0.12): her turn, with no way to get it wrong - if she waits, a
   gentle reminder, and then it goes on by itself */
enum { BALL_DEVA, BALL_TO_KING, BALL_KING, BALL_TO_DEVA };
#define ACT_REMIND (6 * FPS) /* waiting this long: the reminder, and the second time it goes on alone */
#define ACT_AUTO_GAP 45      /* ...an action every so often */
#define BALL_FLY 30          /* frames of a throw */
#define NOTE_FLY 40          /* frames of a note of the lullaby flying to the ogre */
#define ROCKET 26            /* frames of a firework going up */
#define LANTERN_FLY 50       /* frames of the lantern flying to the wizard */
static const struct {
    const char *name;   /* for the log */
    int need;           /* actions to do */
    int gap;            /* frames between two actions, at least */
    const char *remind; /* said when she waits */
} ACTS[] = {
    {"", 0, 0, NULL},
    {"dance", 4, 24, "fn_balla"},
    {"lantern", 3, 24, "st2_lanterna"},
    {"lullaby", 7, 16, "st3_suona"},
    {"ball", 3, 0, "re_tocca_te"},
    {"fireworks", 5, 14, "ep_fuochi"},
};
static const char *const NOTE_SFX[5] = {"nota_do", "nota_re", "nota_mi", "nota_fa", "nota_sol"};
static const char *const NOTE_SPRITE[5] = {"nota_rossa", "nota_arancione", "nota_azzurra", "nota_verde", "nota_oro"};
static const int MELODY[7] = {0, 1, 2, 3, 2, 1, 0}; /* the lullaby on the four notes found: do re mi fa mi re do */
/* the dance with the witch: the moves of "Balla con me" (the red button: a little jump) */
static const struct {
    int btn;
    hero_pose_t pose;
    bool flip;
    const char *tone;
} DANCE[5] = {{BTN_UP, POSE_UP, false, "tone_su"},
              {BTN_DOWN, POSE_GIU, false, "tone_giu"},
              {BTN_LEFT, POSE_STEP, false, "tone_sx"},
              {BTN_RIGHT, POSE_STEP, true, "tone_dx"},
              {BTN_A, POSE_JUMP, false, "boing"}};

static void act_reset(void)
{
    R.act_ready = false;
    R.act_n = 0;
    R.act_t = 999;
    R.act_quiet = 0;
    R.act_reminds = 0;
    R.act_auto = false;
    R.act_done_t = -1;
    R.copy_t = -1;
    R.light_r = 30.0f;
    R.light_target = 30;
    R.lantern_t = -1;
    for (int i = 0; i < MAX_FLY; i++)
        R.fly_t[i] = -1;
    R.ball = BALL_DEVA;
    R.ball_t = 0;
    R.king_talk = 0;
    R.wait_t = 999;
}

static int fly_slot(void)
{
    for (int i = 0; i < MAX_FLY; i++)
        if (R.fly_t[i] < 0)
            return i;
    return 0;
}

static bool flying(void)
{
    for (int i = 0; i < MAX_FLY; i++)
        if (R.fly_t[i] >= 0)
            return true;
    return false;
}

static void act_done(int kind)
{
    R.act_done_t = 0;
    BOT("racconto act=%s done\n", ACTS[kind].name);
}

/* where the light of the second ending is: the star of her wand, then the lantern */
static void lantern_pos(int *x, int *y)
{
    int hx, hy;
    hero_hand(&R.deva, 2, &hx, &hy);
    int sx = hx + 2, sy = hy - 40;
    if (R.lantern_t < 0) {
        *x = sx, *y = sy;
        return;
    }
    int k = clampi(R.lantern_t * 256 / LANTERN_FLY, 0, 256);
    *x = sx + (204 - sx) * k / 256;
    *y = sy + (126 - sy) * k / 256 - (k * (256 - k)) / 1200;
}

static void act_action(int kind, int which)
{
    R.act_n++;
    R.act_t = 0;
    R.act_quiet = 0;
    switch (kind) {
    case ACT_DANCE: /* she does a move, the witch copies it a moment later */
        R.move = which;
        hero_move(&R.deva, DANCE[which].pose, DANCE[which].flip, 26);
        sfx(DANCE[which].tone);
        R.copy_t = -12;
        break;
    case ACT_LANTERN: { /* a wave of the wand: more light, and at the third the lantern */
        static const int LIGHT[4] = {30, 50, 74, 96};
        int x, y;
        lantern_pos(&x, &y);
        sfx("incantesimo");
        fx_sparkles(x, y, 16 + R.act_n * 6, 14);
        R.light_target = LIGHT[clampi(R.act_n, 0, 3)];
        if (R.act_n >= ACTS[kind].need) {
            R.lantern_t = 0;
            sfx("sparkle");
        }
        break;
    }
    case ACT_LULLABY: { /* the next note of the song flies to the ogre */
        int i = fly_slot(), note = MELODY[clampi(R.act_n - 1, 0, 6)];
        R.fly_t[i] = 0;
        R.fly_k[i] = note;
        sfx(NOTE_SFX[note]);
        if (R.act_n == 3)
            sfx("yawn");
        break;
    }
    case ACT_BALL: /* a throw to the king */
        R.ball = BALL_TO_KING;
        R.ball_t = 0;
        sfx("pop");
        break;
    case ACT_FIREWORKS: {
        int i = fly_slot();
        R.fly_t[i] = 0;
        R.fly_k[i] = rng_range(0, 4);
        R.fly_x[i] = rng_range(40, 280);
        R.fly_y[i] = rng_range(26, 86);
        sfx("whoosh");
        break;
    }
    }
    BOT("racconto act=%s n=%d/%d%s\n", ACTS[kind].name, R.act_n, ACTS[kind].need, R.act_auto ? " auto" : "");
}

static void ball_wait(void) /* the red button while it is the king's turn */
{
    if (R.wait_t > 90 && !voice_busy()) {
        say("re_aspetta");
        R.king_talk = 80;
        R.wait_t = 0;
        BOT("racconto act=ball aspetta\n");
    }
}

static void act_update(const rstep_t *s)
{
    int kind = s->act;
    R.act_t++;
    R.wait_t++;
    if (R.king_talk > 0)
        R.king_talk--;
    if (R.copy_t < 40)
        R.copy_t++;
    R.light_r += ((float)R.light_target - R.light_r) / 6.0f;
    /* what flies: the notes reach the ogre, the fireworks burst */
    for (int i = 0; i < MAX_FLY; i++) {
        if (R.fly_t[i] < 0)
            continue;
        R.fly_t[i]++;
        if (kind == ACT_FIREWORKS && R.fly_t[i] == ROCKET) {
            static const uint16_t COL[5] = {0xfb36, 0xfe45, 0x4ed8, 0x75df, 0xb3bd};
            fx_burst(R.fly_x[i], R.fly_y[i], 18);
            fx_sparkles(R.fly_x[i], R.fly_y[i], 26, 16);
            sfx(R.fly_k[i] & 1 ? "pop" : "sparkle");
            (void)COL;
        }
        if ((kind == ACT_FIREWORKS && R.fly_t[i] >= ROCKET + 2) || (kind != ACT_FIREWORKS && R.fly_t[i] >= NOTE_FLY)) {
            if (kind == ACT_LULLABY)
                fx_sparkles(214, 78, 10, 5);
            R.fly_t[i] = -1;
        }
    }
    if (R.lantern_t >= 0 && R.lantern_t < LANTERN_FLY)
        R.lantern_t++;
    if (R.act_done_t >= 0) {
        R.act_done_t++;
        return;
    }
    if (!R.act_ready) { /* first the prompt, then her turn */
        if (R.t > 20 && !voice_busy()) {
            R.act_ready = true;
            BOT("racconto act=%s ready\n", ACTS[kind].name);
            if (kind == ACT_BALL)
                BOT("racconto act=ball turn=deva\n");
        }
        return;
    }
    int need = ACTS[kind].need;
    bool more = R.act_n < need, can = more && R.act_t >= ACTS[kind].gap, acted = false;
    if (kind == ACT_BALL) {
        switch (R.ball) {
        case BALL_DEVA:
            if (btn_pressed(BTN_A)) {
                act_action(kind, 0);
                acted = true;
            }
            break;
        case BALL_TO_KING:
            if (++R.ball_t >= BALL_FLY) { /* caught: "tocca a me!" */
                R.ball = BALL_KING;
                R.ball_t = 0;
                sfx("pop");
                fx_hearts(212, 132, R.act_n >= need ? 6 : 1);
                say("re_tocca_me");
                R.king_talk = 60;
                BOT("racconto act=ball turn=king\n");
                if (R.act_n >= need) {
                    sfx("star");
                    act_done(kind);
                    return;
                }
            } else if (btn_pressed(BTN_A)) {
                ball_wait();
            }
            break;
        case BALL_KING:
            if (!voice_busy() && ++R.ball_t >= 40) { /* he has said it, he waits a moment: back to her */
                R.ball = BALL_TO_DEVA;
                R.ball_t = 0;
                sfx("pop");
            } else if (btn_pressed(BTN_A)) {
                ball_wait();
            }
            break;
        case BALL_TO_DEVA:
            if (++R.ball_t >= BALL_FLY) {
                R.ball = BALL_DEVA;
                R.ball_t = 0;
                R.act_quiet = 0;
                R.act_t = 0;
                say("re_tocca_te");
                R.king_talk = 50;
                BOT("racconto act=ball turn=deva\n");
            }
            break;
        }
        if (R.ball != BALL_DEVA)
            return; /* the waiting below is for her turn */
    } else if (kind == ACT_DANCE) {
        for (int d = 0; d < 5; d++)
            if (btn_pressed(DANCE[d].btn) && can) {
                act_action(kind, d);
                acted = true;
                break;
            }
    } else if (btn_pressed(BTN_A) && can) {
        act_action(kind, 0);
        acted = true;
    }
    /* she waits: a reminder, then it goes on by itself */
    if (more && !acted) {
        if (!voice_busy())
            R.act_quiet++;
        if (R.act_quiet >= ACT_REMIND) {
            R.act_quiet = 0;
            if (R.act_reminds < 1) {
                say(ACTS[kind].remind);
                if (kind == ACT_BALL)
                    R.king_talk = 50;
                R.act_reminds++;
                BOT("racconto act=%s remind\n", ACTS[kind].name);
            } else if (!R.act_auto) {
                R.act_auto = true;
                BOT("racconto act=%s auto\n", ACTS[kind].name);
            }
        }
        if (R.act_auto && R.act_t >= imax(ACT_AUTO_GAP, ACTS[kind].gap) && !voice_busy())
            act_action(kind, rng_range(0, 3));
    }
    /* done? */
    switch (kind) {
    case ACT_DANCE:
        if (R.act_n >= need && R.copy_t >= 30) { /* she has copied the last move too: they laugh */
            fx_hearts(190, 120, 6);
            sfx("star");
            act_done(kind);
        }
        break;
    case ACT_LANTERN:
        if (R.lantern_t >= LANTERN_FLY) { /* the lantern is in his hands: the light fills the tower */
            R.light_target = 420;
            fx_sparkles(204, 120, 30, 20);
            act_done(kind);
        }
        break;
    case ACT_LULLABY:
        if (R.act_n >= need && !flying()) { /* the last note has reached him: asleep */
            sfx("ronfo");
            fx_zed(238 + 20, 110);
            act_done(kind);
        }
        break;
    case ACT_FIREWORKS:
        if (R.act_n >= need && !flying())
            act_done(kind);
        break;
    default: break;
    }
}

static void start_step(void)
{
    const rstep_t *s = cur();
    char id[40];
    R.t = R.after = 0;
    R.reveal_t = 0;
    /* a gesture that began before this step does not skip it, unless the grown-ups are skipping */
    R.hold_skip = grownup_held() ? (R.fast ? 0 : -1) : 0;
    if (R.step > 0 && R.tale->steps[R.step - 1].cast == CAST_WAND)
        R.wand_given = true;
    act_reset();
    if (s->bg) {
        static char place[32];
        char was[32];
        snprintf(was, sizeof(was), "%s", R.bg ? R.bg : "");
        if (!strcmp(s->bg, "@")) {
            snprintf(place, sizeof(place), "bg_%s", story_ch(R.ch)->place);
            R.bg = place;
        } else {
            R.bg = s->bg;
        }
        if (was[0] && strcmp(was, R.bg) && !trans_active()) /* a new place in the tale: the pictures melt */
            trans_begin(TR_FADE);
    }
    if (s->music && !strcmp(s->music, "-")) /* silence: the lullaby of the notes, alone */
        music_stop();
    else if (s->music) /* "mappa" = the music of this adventure's map */
        music_play(snd_find(SND_MUSIC, strcmp(s->music, "mappa") ? s->music : story_arc_def()->music));
    if (s->sfx) {
        if (!strcmp(s->sfx, "@")) /* the villain comes with thunder, the ogre stamps his feet, the king toots */
            sfx(story_villain(R.ch) ? (story_arc() == ARC_MUSICA ? "pestone" : story_arc() == ARC_GIOCHI ? "trombetta" : "tuono")
                                    : "ruggito");
        else
            sfx(s->sfx);
    }
    if (s->voice) {
        resolve(s->voice, id, sizeof(id));
        say(id);
    } else {
        voice_stop();
    }
    switch (s->fx) {
    case FX_SHAKE: R.shake = 24; break;
    case FX_POOF: fx_sparkles(250, 150, 30, 24); break;
    case FX_TRANSFORM: fx_sparkles(230, 150, 50, 40); break;
    case FX_CONFETTI: fx_confetti(160, 60, 60); break;
    case FX_SPARKLE: fx_sparkles(160, 100, 90, 30); break;
    case FX_LIGHTNING:
    case FX_GREY:
    case FX_LIGHT:
    case FX_NONE: break;
    }
    if (s->cast == CAST_PARTY || s->cast == CAST_PARTY2 || s->cast == CAST_PARTY3 || s->cast == CAST_PARTY4 ||
        s->cast == CAST_GEM || s->cast == CAST_EP_PARTY)
        hero_play(&R.deva, HA_DANCE, 0);
    else if (s->cast == CAST_MAGO || s->cast == CAST_WAND || s->cast == CAST_GRISELLA || s->cast == CAST_ORCO_FRIEND ||
             s->cast == CAST_EP_FRIEND)
        hero_play(&R.deva, HA_WAVE, 90);
    else if (s->cast == CAST_SKY || s->cast == CAST_EP_INVITE)
        hero_play(&R.deva, HA_OH, 60);
    else
        hero_play(&R.deva, HA_IDLE, 0);
    BOT("racconto %s step=%d\n", R.tale->name, R.step + 1);
}

static void next_step(void)
{
    do
        R.step++;
    while (R.step < R.tale->n && !step_wanted(&R.tale->steps[R.step]));
    if (R.step < R.tale->n) {
        start_step();
        return;
    }
    if (s_replay >= 0) { /* told again: nothing to remember */
        BOT("racconto_end replay_%s\n", R.tale->name);
        int then = R.id == R_PROLOGO ? R_FINALE /* ...and now the ending of the same adventure */
                   : (R.id == R_FINALE && s_replay == ARC_COUNT - 1 && G.prog.epilogue == 2) ? R_EPILOGO /* the party */
                                                                                           : -1;
        if (then >= 0) {
            R.id = then;
            R.tale = &TALES[s_replay][then];
            R.ch = CH_VILLAIN;
            R.step = -1;
            R.wand_given = true;
            next_step();
            return;
        }
        s_replay = -1;
        game_goto(R.next);
        return;
    }
    /* told: remember it, and go on */
    if (R.id == R_PROLOGO)
        story_mark_seen(SEEN_PROLOGO);
    if (R.id == R_SFIDA)
        story_mark_seen(SEEN_REGOLE);
    if (R.id == R_FINALE)
        story_mark_seen(SEEN_FINALE);
    if (R.id == R_EPILOGO)
        G.prog.epilogue = 2;
    game_save();
    BOT("racconto_end %s\n", R.tale->name);
    if (R.next == SC_MAPPA)
        mappa_open(R.id == R_VITTORIA ? MAP_FREED : MAP_PLAIN);
    else if (R.next == SC_SFIDA)
        sfida_start();
    else
        game_goto(R.next);
}

static void enter(void)
{
    hero_init(&R.deva, 72, 214);
    R.step = -1;
    R.wand_given = R.id != R_PROLOGO || story_arc() > 0; /* the first prologue gives it to her */
    next_step();
}

/* 0.9.0: the sparkling details of the tale - the wand twinkles in her hand, a
   freed friend sends hearts, the gem leaves a trail, a hug makes hearts */
static bool deva_has_wand(const rstep_t *s)
{
    switch (s->cast) {
    case CAST_FOE:
    case CAST_FOE_MAGO:
    case CAST_FREE:
    case CAST_GEM:
    case CAST_FRIENDS:
    case CAST_SCARED:
    case CAST_LANTERN:
    case CAST_WIZ_GOOD:
    case CAST_MEZZANOTTE:
    case CAST_FRIENDS3:
    case CAST_OGRE_GRUMPY:
    case CAST_OGRE_TIRED:
    case CAST_LULLABY:
    case CAST_OGRE_SLEEP:
    case CAST_OGRE_GOOD:
    case CAST_ORCO_FRIEND:
    case CAST_FRIENDS4:
    case CAST_KING_CRY:
    case CAST_KING_GOOD: return true;
    case CAST_SECRET:
    case CAST_NOTES_READY:
    case CAST_EP_INVITE:
    case CAST_EP_FRIEND:
    case CAST_EP_MAGO: return true;
    case CAST_MAGO: return R.wand_given;
    default: return false;
    }
}

static void sparkle_cast(const rstep_t *s)
{
    if (deva_has_wand(s) && R.t % 50 == 25) {
        int hx, hy;
        hero_hand(&R.deva, 2, &hx, &hy);
        fx_twinkle(hx + rng_range(-2, 2), hy - 34, 0);
    }
    switch (s->cast) {
    case CAST_FREE:
    case CAST_WITCH_GOOD:
    case CAST_WIZ_GOOD:
    case CAST_OGRE_GOOD:
    case CAST_KING_GOOD:
        if (R.t < 50 && R.t % 4 == 0) { /* the spell breaks: twinkles all around */
            int dx, dy;
            rng_pair(-30, 30, -50, 40, &dx, &dy);
            fx_twinkle(238 + dx, 170 + dy, 0);
        }
        if (R.t == 50) { /* good again: a burst of stars and hearts */
            fx_burst(238, 150, 14);
            fx_hearts(238, 140, 5);
        }
        break;
    case CAST_GEM: { /* the gem flies to Deva, with a trail */
        int k = clampi(R.t * 256 / 50, 0, 256);
        if (k < 256 && R.t % 3 == 0) {
            int dx, dy;
            rng_pair(-3, 3, -3, 3, &dx, &dy);
            fx_twinkle(230 + (72 - 230) * k / 256 + dx, 110 + (60 - 110) * k / 256 + dy, 0);
        }
        if (R.t == 50)
            fx_burst(72, 60, 12);
        if (R.t % 80 == 60)
            fx_hearts(72, 150, 2);
        break;
    }
    case CAST_HUG:
        if (R.t > 60 && R.t % 25 == 5) /* a hug makes hearts */
            fx_hearts(172, 120, 1);
        break;
    case CAST_PARTY:
    case CAST_PARTY2:
    case CAST_PARTY3:
    case CAST_PARTY4:
        if (R.t % 70 == 35)
            fx_hearts(160, 150, 2);
        break;
    case CAST_VALLEY: /* the music of the valley: twinkles in the air */
    case CAST_TOYLAND: /* the toys at play */
    case CAST_TOYS_BACK:
        if (R.t % 6 == 0) {
            int x, y;
            rng_pair(10, 310, 30, 200, &x, &y);
            fx_twinkle(x, y, 0);
        }
        break;
    case CAST_OGRE_HILL: /* every stamp shakes the valley ("pestone" is two stamps: bum, bum) */
        if (R.t % 90 == 0 && R.t > 0)
            sfx("pestone");
        if (R.t % 90 == 0 || R.t % 90 == 25)
            R.shake = 8;
        break;
    case CAST_SKY: /* the stars go back to the sky */
        if (R.t % 3 == 0) {
            int x, y;
            rng_pair(10, 310, 6, 90, &x, &y);
            fx_twinkle(x, y, 0);
        }
        break;
    case CAST_WAND: /* the wand flies from his hand to hers */
        if (R.t < 60 && R.t % 3 == 0) {
            int k = clampi(R.t * 256 / 60, 0, 256), hx, hy;
            hero_hand(&R.deva, 2, &hx, &hy);
            fx_twinkle(196 + (hx - 196) * k / 256, 130 + (hy - 36 - 130) * k / 256 - (k * (256 - k)) / 600 + 10, 0);
        }
        break;
    default: break;
    }
}

/* ---- the fourth adventure: the toy train on the map, the game of turns */

/* where the toy train is at a frame: it runs along the track (the dotted path of the map) from the
   tent to the castle and back, a pixel and a half a frame; left = it goes leftwards */
static void train_pos(uint32_t frame, int *x, int *y, bool *left)
{
    int mx[6], my[6], len[5], total = 0;
    map_points(mx, my);
    for (int i = 0; i < 5; i++) {
        float dx = (float)(mx[i + 1] - mx[i]), dy = (float)(my[i + 1] - my[i]);
        len[i] = imax(1, (int)sqrtf(dx * dx + dy * dy));
        total += len[i];
    }
    int d = (int)((frame * 3u / 2u) % (uint32_t)(2 * total));
    bool back = d >= total;
    if (back)
        d = 2 * total - d;
    int i = 0;
    while (i < 4 && d > len[i]) {
        d -= len[i];
        i++;
    }
    d = imin(d, len[i]);
    *x = mx[i] + (mx[i + 1] - mx[i]) * d / len[i];
    *y = my[i] + (my[i + 1] - my[i]) * d / len[i] - 2;
    *left = back ? mx[i + 1] > mx[i] : mx[i + 1] < mx[i];
}

/* the spell spreads from the king's castle (FX_GREY): has it reached the train? */
static bool train_caught(void)
{
    int mx[6], my[6], x, y;
    bool left;
    map_points(mx, my);
    train_pos(R.train_frame, &x, &y, &left);
    int dx = x - mx[CH_COUNT], dy = y - (my[CH_COUNT] - 20), r = R.t * 4;
    return dx * dx + dy * dy <= r * r;
}

static void draw_train(bool grey)
{
    int x, y;
    bool left;
    train_pos(R.train_frame, &x, &y, &left);
    const sprite_t *t = gfx_sprite("trenino");
    if (grey)
        gfx_set_tint(rgb565(0x9a, 0x98, 0xa8), 210);
    gfx_blit(t, x - t->w / 2, y - t->h + 3 - (grey ? 0 : (int)((G.frame / 6) & 1)), left ? GFX_FLIP_H : 0);
    gfx_set_tint(0, 0);
}

/* "un po' tu e un po' io": the ball goes from Deva to the king and back, a pass a second */
/* the treasures of the prologue: born by the villain ("...cinque gemme magiche"), or given away */
static bool born_cast(int c) { return c == CAST_GEMS_BORN || c == CAST_STARS_BORN || c == CAST_NOTES_BORN || c == CAST_KEYS_BORN; }
static bool give_cast(int c) { return c == CAST_GEMS || c == CAST_STARS || c == CAST_NOTES || c == CAST_KEYS; }

static void update(void)
{
    R.t++;
    hero_update(&R.deva);
    R.deva.talking = false;
    if (R.shake > 0)
        R.shake--;
    const rstep_t *s = cur();
    if (s->fx == FX_LIGHTNING && (R.t == 1 || R.t == 9))
        sfx(R.t == 1 ? "tuono" : "pop");
    if (s->cast == CAST_TOYLAND || s->cast == CAST_KING_HOME || s->cast == CAST_TOYS_BACK)
        R.train_frame = G.frame; /* the toy train runs */
    if (s->cast == CAST_KING_SPELL && !train_caught())
        R.train_frame = G.frame; /* ...until the spell catches it */
    if ((s->cast == CAST_TOYLAND || s->cast == CAST_TOYS_BACK) && R.t % 200 == 60)
        sfx("treno"); /* tu-tuu! */
    if (born_cast(s->cast)) { /* the five treasures pop out by the villain, one by one */
        const int *c = BORN_CUES[story_arc()];
        for (int i = 0; i < CH_COUNT; i++)
            if (R.t == c[1] + i * 8)
                sfx(s->cast == CAST_NOTES_BORN ? NOTE_SFX[i] : "pop");
    }
    if (give_cast(s->cast)) {
        const int *c = GIVE_CUES[story_arc()];
        int mx[6], my[6];
        map_points(mx, my);
        if (R.t == c[0]) { /* the four monsters fall under the spell */
            sfx("incantesimo");
            for (int i = 1; i < CH_COUNT; i++)
                fx_sparkles(mx[i] + 10, my[i] - 30, 18, 10);
        }
        if (R.t == c[1])
            sfx("whoosh");
        if (R.t == c[2])
            sfx("sparkle");
    }
    sparkle_cast(s);
    if (s->cast == CAST_OGRE_SLEEP && R.t % 90 == 30) {
        sfx("ronfo");
        fx_zed(238 + 20, 110);
    }
    if (s->look == LOOK_REVEAL)
        R.reveal_t++;
    if (voice_busy())
        R.after = 0;
    else
        R.after++;
    bool done;
    if (s->act) { /* her turn: the step goes on when the kind act is done */
        act_update(s);
        done = R.act_done_t >= s->hold && !voice_busy();
    } else {
        done = R.t > 40 && R.after >= s->hold;
        /* the red button goes on only when the line is over (pressed in a hurry, it would cut the story;
           0.14: held down it does not skip any more - a child holds it); the grown-ups skip a line with
           L + R held for a second; told again from the album, the red button skips at once */
        if (!grownup_held())
            R.fast = false;
        if (R.hold_skip >= 0)
            R.hold_skip = grownup_held() ? R.hold_skip + 1 : 0;
        else if (!grownup_held())
            R.hold_skip = 0;
        bool skip = false;
        if (R.t > 20) {
            if (racconto_replaying())
                skip = btn_pressed(BTN_A);
            else if (R.hold_skip >= FPS)
                skip = R.fast = true;
            else
                skip = btn_pressed(BTN_A) && !voice_busy();
        }
        if (skip) {
            voice_stop();
            done = true;
        }
    }
    if (done)
        next_step();
}

/* ---- drawing */

/* 0 .. amp and back: a smooth float (0.13; it used to jump between the two in one frame) */
static int bob(int speed, int amp) { return swing((int)G.frame, speed * 4, amp); }

/* the voice of a character drawn: the speaker of this step, while the voice speaks (0.13) */
static int voice_of(const char *sprite)
{
    static const char *const WHO[] = {NULL, "mago", "strega", "mo_", "stregone", "orco", "re_"};
    if (!R.tale || !voice_busy())
        return 0;
    int sp = cur()->speaker;
    if (sp <= SP_NONE || sp >= ARRAY_LEN(WHO) || strncmp(sprite, WHO[sp], strlen(WHO[sp])))
        return 0;
    return voice_level();
}

/* a character standing on the floor, feet at (cx, feet), k times bigger; the characters live
   (they breathe, blink, bounce with their voice: anim.c), the props stand still */
static void stand(const char *sprite, int cx, int feet, int k, int flags)
{
    if (actor_is_alive(sprite)) {
        actor_draw(sprite, cx, feet, k, flags, voice_of(sprite), 0, 0);
        return;
    }
    const sprite_t *s = gfx_sprite(sprite);
    gfx_blit_scaled(s, cx - s->w * k / 2, feet - s->h * k, k, flags);
}

/* the mouth open on the loud syllables of its own voice (0.13: it used to flap every 6 frames) */
static bool talking(int who)
{
    return cur()->speaker == who && voice_busy() && voice_level() > 80;
}

static void draw_deva(int k, bool wand)
{
    hero_draw_zoom(&R.deva, &G.prog, k);
    if (wand) {
        int hx, hy;
        hero_hand(&R.deva, k, &hx, &hy);
        const sprite_t *w = gfx_sprite("bacchetta");
        gfx_blit_scaled(w, hx - 4 * k, hy - 18 * k, k, 0);
    }
}

static void draw_foe(int ch, bool good, int cx, int feet, int k)
{
    const arc_t *a = story_arc_def();
    const char *name;
    bool villain = story_villain(ch);
    if (good) /* (a freed villain has its own ending, see FINALE) */
        name = villain ? a->v_good : story_foe_sprite(ch, 'b');
    else if (villain)
        name = (talking(SP_STREGA) || talking(SP_WIZ) || talking(SP_FOE)) ? a->v_talk : a->v_bad;
    else
        name = story_foe_sprite(ch, talking(SP_FOE) ? 'r' : 'c');
    int dy = (!good && !villain) ? bob(20, 2) : 0;
    stand(name, cx, feet - dy, k, 0);
}

/* a region of the map (the circle that a freed place lights up) */
static void region(int i, int *x, int *y, int *r)
{
    const arc_t *a = story_arc_def();
    char n[24];
    int dummy;
    *x = a->reg[i][0], *y = a->reg[i][1], *r = a->reg[i][2];
    snprintf(n, sizeof(n), "%sreg%d", a->anchor, i);
    gfx_anchor(n, x, y);
    snprintf(n, sizeof(n), "%srad%d", a->anchor, i);
    gfx_anchor(n, r, &dummy);
}

/* where the map's places are, for the tales told over the map */
static void map_points(int x[6], int y[6])
{
    const arc_t *a = story_arc_def();
    for (int i = 0; i < 6; i++) {
        char n[24];
        x[i] = a->loc[i][0];
        y[i] = a->loc[i][1];
        snprintf(n, sizeof(n), "%sloc%d", a->anchor, i);
        gfx_anchor(n, &x[i], &y[i]);
    }
}

/* a character at a party: it hops now and then, crouching, stretched in the air, squashed when it lands
   (0.13: everybody used to jump up and down in step, 2 or 3 pixels at a time) */
static void party_hop(const char *sprite, int cx, int feet, int i, int every)
{
    const sprite_t *s = gfx_sprite(sprite);
    int lift, sdy, sdx;
    hop_every((int)G.frame, (uint32_t)i * 53u + 7u, every + (i * 13) % 31, 26, 7, s->h, &lift, &sdy, &sdx);
    actor_draw(sprite, cx, feet - lift, 1, 0, 0, sdy, sdx);
}

static void draw_friends_party(const char *const back[4], const char *villain_a, const char *villain_b, int feet)
{
    static const int BX[4] = {48, 118, 202, 272};
    for (int i = 0; i < 4; i++)
        party_hop(back[i], BX[i], feet - 34, i, 70);
    party_hop("mago", 70, feet, 5, 120);
    party_hop((G.frame / 20) & 1 ? villain_a : villain_b, 250, feet, 6, 90);
    R.deva.x = 160;
    R.deva.y = feet;
    hero_draw(&R.deva, &G.prog);
    if (R.t % 50 == 0)
        fx_confetti(rng_range(60, 260), 40, 20);
}

static void cast_draw(const rstep_t *s)
{
    int map_x[6], map_y[6];
    map_points(map_x, map_y);
    switch (s->cast) {
    case CAST_NONE:
        break;
    case CAST_FLY: { /* the witch crosses the sky on her broom */
        int x = 340 - R.t * 3, y = 40 + (int)(8 * ((R.t / 10) % 2));
        stand("strega_volo", x, y + 70, 1, 0);
        break;
    }
    case CAST_WITCH_MAP:
        stand("strega_volo", 160, 120 - bob(16, 2), 1, 0);
        break;
    case CAST_GEMS_BORN:  /* "Tutti i colori finirono in cinque gemme magiche": the villain flies home */
    case CAST_STARS_BORN: /* (the witch, the wizard; the ogre and the king are at home already) and */
    case CAST_NOTES_BORN: /* the five treasures pop out by it, one by one; */
    case CAST_KEYS_BORN:
    case CAST_GEMS:       /* then the spell on the four monsters, the four treasures fly to them, */
    case CAST_STARS:      /* the last one stays with the villain */
    case CAST_NOTES:
    case CAST_KEYS: {
        int arc = story_arc();
        bool born = born_cast(s->cast), flies = arc == ARC_GRIGIO || arc == ARC_NOTTE;
        const int *bc = BORN_CUES[arc], *gc = GIVE_CUES[arc];
        int spell = born ? 1 << 20 : gc[0], give = born ? 1 << 20 : gc[1], own = born ? 1 << 20 : gc[2];
        for (int i = 0; i < CH_VILLAIN; i++) { /* good, until the spell (the toys, stopped and grey) */
            bool bad = R.t >= spell;
            const sprite_t *m = gfx_sprite(story_foe_sprite(i, bad ? 'c' : 'b'));
            if (arc == ARC_GIOCHI && !bad)
                gfx_set_tint(rgb565(0x9a, 0x98, 0xa8), 210);
            gfx_blit(m, map_x[i + 1] + 10 - m->w / 2, map_y[i + 1] - m->h - (bad ? bob(20, 2) : 0), 0);
            gfx_set_tint(0, 0);
        }
        int k = born && flies ? clampi(R.t * 256 / imax(1, bc[0]), 0, 256) : 256;
        int vx = 160 + (map_x[CH_COUNT] - 160) * k / 256, vy = 120 + (map_y[CH_COUNT] - 120) * k / 256;
        if (arc == ARC_GIOCHI)
            stand(R.t >= spell && R.t < give ? "re_cp" : "re_c", map_x[CH_COUNT] + 8, map_y[CH_COUNT] + 4, 1, 0);
        else if (arc == ARC_MUSICA)
            stand(R.t >= spell && R.t < give ? "orco_cp" : "orco_c", map_x[CH_COUNT] + 8, map_y[CH_COUNT] + 4, 1, 0);
        else
            stand(arc == ARC_NOTTE ? "stregone_volo" : "strega_volo", vx, vy - bob(16, 2), 1, 0);
        for (int i = 0; i < CH_COUNT; i++) {
            bool kept = i == CH_VILLAIN;
            const sprite_t *g = gfx_sprite(story_ch(i)->gem);
            int sx = vx - 72 + i * 15, sy = vy - 56 + (i - 2) * (i - 2) * 3; /* in a row by the villain */
            if (born) {
                int t = R.t - (bc[1] + i * 8);
                if (t == 0)
                    fx_sparkles(sx, sy, 8, 6);
                if (t >= 0)
                    gfx_blit(g, sx - g->w / 2, sy - g->h / 2 - (t < 6 ? 6 - t : bob(12 + i * 2, 2)), 0);
                continue;
            }
            int t0 = kept ? own : give;
            if (R.t < t0) {
                gfx_blit(g, sx - g->w / 2, sy - g->h / 2 - bob(12 + i * 2, 2), 0);
                continue;
            }
            int tx = kept ? vx - 44 : map_x[i + 1] + 10, ty = kept ? vy - 56 : map_y[i + 1] - 84 + g->h / 2;
            int kk = clampi((R.t - t0) * 256 / 60, 0, 256);
            int x = sx + (tx - sx) * kk / 256, y = sy + (ty - sy) * kk / 256 - (kk * (256 - kk)) / 700;
            gfx_blit(g, x - g->w / 2, y - g->h / 2 - (kk == 256 ? bob(14, 2) : 0), 0);
        }
        break;
    }
    case CAST_MAGO:
    case CAST_WAND:
        draw_deva(2, s->cast == CAST_MAGO && R.wand_given);
        stand(talking(SP_MAGO) ? "mago_p" : "mago", 238, 216, 2, 0);
        if (s->cast == CAST_WAND) { /* the wand flies from his hand to hers */
            int k = clampi(R.t * 256 / 60, 0, 256);
            int hx, hy;
            hero_hand(&R.deva, 2, &hx, &hy);
            int x = 196 + (hx - 196) * k / 256, y = 130 + (hy - 36 - 130) * k / 256 - (k * (256 - k)) / 600;
            gfx_blit_scaled(gfx_sprite("bacchetta"), x - 8, y, 2, 0);
            if (R.t % 6 == 0)
                fx_sparkles(x, y + 10, 8, 2);
        }
        break;
    case CAST_FOE:
    case CAST_FOE_MAGO:
        draw_deva(2, true);
        draw_foe(R.ch, false, 238, 216, 2);
        if (s->cast == CAST_FOE_MAGO) { /* the guide pops in to tell the rules */
            bool talk = talking(SP_MAGO), night = story_arc() == ARC_NOTTE, music = story_arc() == ARC_MUSICA;
            const char *guide = story_arc() == ARC_GIOCHI ? (talk ? "orco_bp" : "orco_b")
                                : music ? (talk ? "stregone_bp" : "stregone_b")
                                        : (night ? (talk ? "strega_bp" : "strega_b") : (talk ? "mago_p" : "mago"));
            stand(guide, 40, 96 - bob(24, 2), 1, 0);
        }
        break;
    case CAST_FREE: { /* the spell breaks: the foe blinks between the two forms */
        draw_deva(2, true);
        bool good = R.t > 50 || ((R.t / 4) & 1);
        draw_foe(R.ch, good, 238, 216, 2);
        break;
    }
    case CAST_GEM: {
        draw_deva(2, true);
        draw_foe(R.ch, true, 238, 216, 2);
        int k = clampi(R.t * 256 / 50, 0, 256);
        int x = 230 + (72 - 230) * k / 256, y = 110 + (60 - 110) * k / 256 - bob(14, 2);
        const sprite_t *g = gfx_sprite(story_ch(R.ch)->gem);
        gfx_blit_scaled(g, x - g->w, y - g->h, 2, 0);
        break;
    }
    case CAST_CRY:
    case CAST_HUG: {
        int dx = s->cast == CAST_HUG ? clampi(R.t, 0, 60) : 0;
        R.deva.x = 72 + dx;
        draw_deva(2, false);
        stand("strega_pianto", 238, 216 - ((R.t / 30) & 1), 2, 0);
        if (s->cast == CAST_HUG && R.t > 60 && R.t % 20 == 0)
            fx_sparkles(190, 110, 20, 3);
        break;
    }
    case CAST_WITCH_GOOD: {
        R.deva.x = 132;
        draw_deva(2, false);
        bool good = R.t > 50 || ((R.t / 4) & 1);
        stand(good ? (talking(SP_STREGA) ? "strega_bp" : "strega_b") : "strega_pianto", 238, 216, 2, 0);
        if (good && R.t > 50)
            gfx_blit_scaled(gfx_sprite("gemma_viola"), 212, 44 - bob(14, 2), 2, 0);
        break;
    }
    case CAST_PARTY: { /* everybody dances on the stage */
        static const char *const BACK[4] = {"mo_ciuffone_b", "mo_melmoso_b", "mo_rocciolo_b", "mo_tuonello_b"};
        draw_friends_party(BACK, "strega_b", "strega_bp", 218);
        break;
    }
    /* ---- the second adventure */
    case CAST_STEAL: { /* the wizard rides his moon across the sky: behind him, no more stars */
        int x = 350 - R.t * 3, y = 44 + (int)(6 * ((R.t / 12) % 2));
        gfx_set_clip(clampi(x + 20, 0, 320), 0, 320, 240);
        gfx_blit_image(gfx_image("bg_mappa2_buia"), 0, 0, 0);
        gfx_reset_state();
        stand("stregone_volo", x, y + 60, 1, 0);
        if (R.t % 5 == 0 && x > 0 && x < 320) /* a star swept into the sack */
            fx_sparkles(x + 18, y + 10, 10, 3);
        break;
    }
    case CAST_WIZ_MAP:
        stand("stregone_volo", 160, 124 - bob(16, 2), 1, 0);
        break;
    case CAST_GRISELLA: { /* Grisella lands on her broom next to Deva */
        draw_deva(2, false);
        int k = clampi(R.t * 256 / 70, 0, 256);
        int x = 330 + (220 - 330) * k / 256, y = 40 + (150 - 40) * k / 256;
        if (k < 256)
            stand("strega_volo_b", x, y, 2, 0);
        else
            stand(talking(SP_STREGA) ? "strega_bp" : "strega_b", 238, 216, 2, 0);
        break;
    }
    case CAST_FRIENDS: /* Deva with the wizard and Grisella flying above */
        draw_deva(2, true);
        stand(talking(SP_MAGO) ? "mago_p" : "mago", 250, 216, 2, 0);
        stand("strega_volo_b", 150, 96 - bob(16, 3), 1, 0);
        if (!strcmp(s->voice, "st2_prologo_6") && R.t % 8 == 0 && R.t < 80) {
            int hx, hy;
            hero_hand(&R.deva, 2, &hx, &hy);
            fx_sparkles(hx, hy - 36, 10, 3);
        }
        break;
    case CAST_SCARED: /* the wizard trembles in the dark */
        draw_deva(2, true);
        stand("stregone_paura", 238 + (((R.t / 2) & 1) ? 1 : -1), 216, 2, 0);
        break;
    case CAST_LANTERN: { /* she waves the wand: more light each time, then the lantern flies to him */
        R.deva.x = 92;
        draw_deva(2, true);
        bool calm = R.act_done_t >= 0;
        stand("stregone_paura", 238 + (!calm && ((R.t / 3) & 1) ? 1 : 0), 216, 2, 0);
        if (R.lantern_t >= 0) {
            int x, y;
            lantern_pos(&x, &y);
            gfx_blit_scaled(gfx_sprite("lanterna"), x - 14, y - 22, 2, 0);
            if (R.t % 12 == 0)
                fx_sparkles(x, y - 8, 12, 2);
        }
        break;
    }
    case CAST_SECRET: { /* "anche lei, a volte, ha paura del buio": the lantern in his hands, a little heart */
        R.deva.x = 132;
        draw_deva(2, true);
        stand("stregone_paura", 238, 216, 2, 0);
        gfx_blit_scaled(gfx_sprite("lanterna"), 204 - 14, 126 - 22 - bob(20, 1), 2, 0);
        if (R.t % 50 == 30)
            fx_hearts(190, 110, 1);
        break;
    }
    case CAST_WIZ_GOOD: {
        R.deva.x = 132;
        draw_deva(2, true);
        bool good = R.t > 50 || ((R.t / 4) & 1);
        stand(good ? (talking(SP_WIZ) ? "stregone_bp" : "stregone_b") : "stregone_paura", 238, 216, 2, 0);
        if (good && R.t > 50)
            gfx_blit_scaled(gfx_sprite("stella_oro"), 204, 40 - bob(14, 2), 2, 0);
        break;
    }
    case CAST_SKY: /* the stars go back to the sky, from the tower outwards (FX_LIGHT) */
        if (R.t % 7 == 0) {
            int x, y;
            rng_pair(20, 300, 8, 70, &x, &y);
            fx_sparkles(x, y, 6, 2);
        }
        break;
    case CAST_PARTY2: { /* a party under the stars, on the beach */
        static const char *const BACK[4] = {"mo_polpone_b", "mo_lumacone_b", "mo_nevone_b", "mo_fumino_b"};
        stand((G.frame / 24) & 1 ? "strega_b" : "strega_bp", 22, 222 - bob(14, 2), 1, 0);
        draw_friends_party(BACK, "stregone_b", "stregone_bp", 222);
        break;
    }
    /* ---- the third adventure */
    case CAST_VALLEY:    /* everybody sings: notes float up from every place */
    case CAST_OGRE_HILL: /* ...and up on his hill the ogre stamps his feet */
    case CAST_OGRE_BLOW: { /* he blows: the notes fly to him, the valley goes quiet */
        static const char *const COLS[5] = {"nota_rossa", "nota_arancione", "nota_azzurra", "nota_verde", "nota_oro"};
        int ox = map_x[CH_COUNT] + 8, oy = map_y[CH_COUNT] + 4;
        for (int i = 1; i <= CH_COUNT; i++) {
            const sprite_t *n = gfx_sprite(COLS[i - 1]);
            int t = (R.t + i * 23) % 90, x = map_x[i] + 10 + ((t / 15) & 1 ? 2 : -2), y = map_y[i] - 40 - t / 3;
            if (s->cast == CAST_OGRE_BLOW) { /* sucked towards the ogre, then gone */
                int k = clampi(R.t * 256 / 70, 0, 256);
                x = x + (ox - 10 - x) * k / 256;
                y = y + (oy - 50 - y) * k / 256;
                if (k >= 256)
                    continue;
            }
            if (i < CH_COUNT || s->cast != CAST_OGRE_BLOW)
                gfx_blit(n, x - n->w / 2, y - n->h / 2, 0);
        }
        if (s->cast == CAST_OGRE_HILL) {
            int hop = (R.t % 30) < 6 ? -4 : 0; /* stamp! */
            stand("orco_c", ox, oy + hop, 1, 0);
        } else if (s->cast == CAST_OGRE_BLOW) {
            stand(talking(SP_ORCO) && (R.t / 40) % 2 ? "orco_cp" : "orco_soffia", ox, oy, 1, 0);
            for (int k = 0; k < 3; k++) { /* puffs of wind */
                int t = (R.t * 3 + k * 40) % 120;
                const sprite_t *p = gfx_sprite(t < 40 ? "puff_0" : "puff_2");
                gfx_blit(p, ox - 30 - t, oy - 50 + k * 8, 0);
            }
        }
        break;
    }
    case CAST_MEZZANOTTE: /* the wizard, good now, pops up next to Deva */
        draw_deva(2, true);
        stand(talking(SP_WIZ) ? "stregone_bp" : "stregone_b", 238, 216, 2, 0);
        break;
    case CAST_FRIENDS3: /* Deva, the Mago Pistacchio, Mezzanotte; Grisella flying above */
        R.deva.x = 64;
        draw_deva(2, true);
        stand(talking(SP_MAGO) ? "mago_p" : "mago", 158, 216, 2, 0);
        stand(talking(SP_WIZ) ? "stregone_bp" : "stregone_b", 258, 216, 2, 0);
        stand("strega_volo_b", 150, 88 - bob(16, 3), 1, 0);
        break;
    case CAST_OGRE_GRUMPY: /* the spell broken, the ogre stamps and puffs */
        draw_deva(2, true);
        stand("orco_brontola", 238, 216 - ((R.t % 24) < 5 ? 3 : 0), 2, 0);
        break;
    case CAST_OGRE_TIRED: /* he yawns: he cannot sleep */
        draw_deva(2, true);
        stand(talking(SP_ORCO) ? "orco_stanco_p" : "orco_stanco", 238, 216, 2, 0);
        break;
    case CAST_NOTES_READY: /* "Deva prende le quattro note magiche": they come round her, one by one */
    case CAST_LULLABY: {   /* ...and she plays them: a note at every press, flying to the ogre */
        R.deva.x = 92;
        draw_deva(2, true);
        bool play = s->cast == CAST_LULLABY;
        const char *ogre = "orco_stanco";
        if (play && R.act_done_t >= 0)
            ogre = "orco_dorme";
        else if (play && R.act_n >= 3 && R.act_n <= 4 && R.act_t < 50)
            ogre = "orco_stanco_p"; /* a big yawn */
        else if (play && R.act_n >= 5)
            ogre = gfx_has_sprite("orco_assonnato") ? "orco_assonnato" : "orco_stanco";
        stand(ogre, 238, 216, 2, 0);
        for (int i = 0; i < 4; i++) { /* do, re, mi, fa round her head */
            int t = play ? 99 : R.t - 10 - i * 8;
            if (t < 0)
                continue;
            if (t == 0)
                fx_sparkles(46 + i * 30, 54, 8, 6);
            bool just = play && R.act_n > 0 && MELODY[clampi(R.act_n - 1, 0, 6)] == i && R.act_t < 14;
            const sprite_t *n = gfx_sprite(NOTE_SPRITE[i]);
            int y = 58 + (i == 0 || i == 3 ? 6 : 0) - (just ? 6 : bob(14 + i * 3, 2)) - (t < 6 ? 6 - t : 0);
            gfx_blit(n, 46 + i * 30 - n->w / 2, y - n->h / 2, 0);
        }
        for (int i = 0; play && i < MAX_FLY; i++) { /* the notes on their way to him */
            if (R.fly_t[i] < 0)
                continue;
            int k = R.fly_k[i], t = R.fly_t[i];
            int sx = 46 + k * 30, sy = 58, tx = 214, ty = 74;
            int x = sx + (tx - sx) * t / NOTE_FLY, y = sy + (ty - sy) * t / NOTE_FLY - (t * (NOTE_FLY - t)) / 18;
            const sprite_t *n = gfx_sprite(NOTE_SPRITE[k]);
            gfx_blit_scaled(n, x - n->w, y - n->h, 2, 0);
        }
        break;
    }
    case CAST_OGRE_SLEEP: /* asleep, at last */
        draw_deva(2, true);
        stand("orco_dorme", 238, 216, 2, 0);
        break;
    case CAST_OGRE_GOOD: { /* he wakes up happy, with the golden note */
        R.deva.x = 132;
        draw_deva(2, true);
        bool good = R.t > 50 || ((R.t / 4) & 1);
        stand(good ? (talking(SP_ORCO) ? "orco_bp" : "orco_b") : "orco_dorme", 238, 216, 2, 0);
        if (good && R.t > 50)
            gfx_blit_scaled(gfx_sprite("nota_oro"), 204, 40 - bob(14, 2), 2, 0);
        break;
    }
    case CAST_PARTY3: { /* a party at the circus: everybody, and the ogre dances too */
        static const char *const BACK[4] = {"mo_caramellone_b", "mo_fungone_b", "mo_ranocchione_b", "mo_trombone_b"};
        stand((G.frame / 24) & 1 ? "strega_b" : "strega_bp", 22, 222 - bob(14, 2), 1, 0);
        stand((G.frame / 20) & 1 ? "stregone_b" : "stregone_bp", 298, 222 - bob(15, 2), 1, 0);
        draw_friends_party(BACK, "orco_b", "orco_bp", 222);
        break;
    }
    /* ---- the fourth adventure */
    case CAST_TOYLAND:    /* the toys are alive: the toy monsters hop at their places, the train runs */
    case CAST_KING_HOME:  /* ...and in his castle the little king stamps his feet */
    case CAST_KING_SPELL: /* "fermi tutti!": the spell spreads from the castle, the toys stop and go grey */
    case CAST_TOYS_BACK: { /* the golden key back: everything moves again, from the castle outwards */
        bool spell = s->cast == CAST_KING_SPELL;
        int cx = map_x[CH_COUNT], cy = map_y[CH_COUNT] - 20, r = R.t * 4;
        for (int i = 0; i < CH_VILLAIN; i++) {
            const sprite_t *m = gfx_sprite(story_foe_sprite(i, 'b'));
            int x = map_x[i + 1] + 10, y = map_y[i + 1];
            int dx = x - cx, dy = y - 30 - cy;
            bool caught = spell && dx * dx + dy * dy <= r * r;
            int hop = caught ? 0 : ((((G.frame / 8) + (uint32_t)i * 3) % 8) < 2 ? -3 : 0);
            if (caught)
                gfx_set_tint(rgb565(0x9a, 0x98, 0xa8), 210);
            gfx_blit(m, x - m->w / 2, y - m->h + hop, 0);
            gfx_set_tint(0, 0);
        }
        draw_train(spell && train_caught());
        if (s->cast == CAST_KING_HOME || spell) {
            int stamp = (R.t % 30) < 6 ? -3 : 0; /* stamp! */
            const char *k = talking(SP_RE) && ((R.t / 20) & 1) ? "re_cp" : "re_c";
            stand(s->cast == CAST_KING_HOME ? ((R.t / 30) & 1 ? "re_cp" : "re_c") : k, map_x[CH_COUNT] + 8,
                  map_y[CH_COUNT] + 4 + stamp, 1, 0);
            if (spell && R.t % 5 == 0 && R.t < 60) /* the magic of his sceptre */
                fx_sparkles(map_x[CH_COUNT] + 20, map_y[CH_COUNT] - 50, 10, 3);
        }
        break;
    }
    case CAST_ORCO_FRIEND: /* the ogre, good now and well rested, comes to help */
        draw_deva(2, true);
        stand(talking(SP_ORCO) ? "orco_bp" : "orco_b", 238, 216, 2, 0);
        break;
    case CAST_FRIENDS4: /* Deva, the Mago Pistacchio and the ogre; Grisella flying above */
        R.deva.x = 56;
        draw_deva(2, true);
        stand(talking(SP_MAGO) ? "mago_p" : "mago", 150, 216, 2, 0);
        stand(talking(SP_ORCO) ? "orco_bp" : "orco_b", 262, 216, 2, 0);
        stand("strega_volo_b", 150, 84 - bob(16, 3), 1, 0);
        break;
    case CAST_KING_CRY: /* the spell broken, the little king cries: nobody plays with him */
        draw_deva(2, true);
        stand("re_piange", 238, 216 - ((R.t / 24) & 1), 2, 0);
        if (R.t % 40 == 10) /* a tear */
            fx_twinkle(238 - 14 + rng_range(-2, 2), 216 - 96, 0);
        break;
    case CAST_BALL_READY: /* "Allora Deva prende una palla": in her hands */
    case CAST_SHARE: {    /* "un po' tu, e un po' io": she throws it, the king throws it back */
        R.deva.x = 92;
        draw_deva(2, false);
        bool play = s->cast == CAST_SHARE, happy = play && (R.act_n > 0 || R.ball != BALL_DEVA);
        bool talk = R.king_talk > 0 && voice_busy() && ((R.t / 6) & 1);
        stand(happy ? "re_gioca" : "re_piange", 238, 216 - (talk ? 2 : 0), 2, 0);
        int hx, hy;
        hero_hand(&R.deva, 2, &hx, &hy);
        int ax = hx + 6, ay = hy - 30, bx = 206, by = 140; /* her hands, the king's */
        int x = ax, y = ay - bob(16, 1);
        if (play && (R.ball == BALL_TO_KING || R.ball == BALL_TO_DEVA)) {
            bool to_king = R.ball == BALL_TO_KING;
            int k = clampi(R.ball_t * 256 / BALL_FLY, 0, 256);
            int x0 = to_king ? ax : bx, y0 = to_king ? ay : by, x1 = to_king ? bx : ax, y1 = to_king ? by : ay;
            x = x0 + (x1 - x0) * k / 256;
            y = y0 + (y1 - y0) * k / 256 - (k * (256 - k)) / 900;
        } else if (play && R.ball == BALL_KING) {
            x = bx;
            y = by - bob(16, 1);
        }
        const sprite_t *b = gfx_sprite("pics_palla");
        gfx_blit_scaled(b, x - b->w, y - b->h, 2, 0);
        break;
    }
    case CAST_KING_GOOD: { /* he laughs, and gives the golden key back */
        R.deva.x = 132;
        draw_deva(2, true);
        bool good = R.t > 50 || ((R.t / 4) & 1);
        stand(good ? (talking(SP_RE) ? "re_bp" : "re_b") : "re_gioca", 238, 216, 2, 0);
        if (good && R.t > 50)
            gfx_blit_scaled(gfx_sprite("chiave_oro"), 204, 40 - bob(14, 2), 2, 0);
        break;
    }
    case CAST_DANCE: { /* she dances with the cross, the witch copies each move a moment later */
        R.deva.x = 132;
        draw_deva(2, false);
        int dx = 0, dy = 0, flags = 0, t = R.copy_t;
        if (R.act_n > 0 && t >= 0 && t < 26) {
            int ph = t < 13 ? t : 26 - t; /* there and back */
            switch (R.move) {
            case 0: dy = -ph; break;                         /* arms up: a hop */
            case 1: dy = ph / 3; break;                      /* down: she bends */
            case 2: dx = -ph / 2; break;                     /* a step to the left */
            case 3: dx = ph / 2, flags = GFX_FLIP_H; break; /* ...to the right */
            default: dy = -ph * 3 / 2; break;                /* the red button: a jump */
            }
        }
        const char *w = R.act_n == 0 ? "strega_pianto" : (gfx_has_sprite("strega_balla") ? "strega_balla" : "strega_c");
        stand(w, 238 + dx, 216 + dy, 2, flags);
        if (R.act_n > 0 && R.t % 24 == 0) { /* the music of the dance */
            int x, y;
            rng_pair(150, 300, 50, 130, &x, &y);
            fx_twinkle(x, y, 0);
        }
        break;
    }
    /* ---- the epilogue: the party for Deva */
    case CAST_EP_INVITE:
        R.deva.x = 160;
        draw_deva(2, true);
        if (R.t % 40 == 20)
            fx_confetti(rng_range(60, 260), 40, 14);
        break;
    case CAST_EP_FRIEND: { /* each old villain, good now, remembers her kind act */
        static const struct {
            const char *voice, *a, *b;
            int sp;
        } WHO[4] = {{"ep_grisella", "strega_b", "strega_bp", SP_STREGA},
                    {"ep_mezzanotte", "stregone_b", "stregone_bp", SP_WIZ},
                    {"ep_orco", "orco_b", "orco_bp", SP_ORCO},
                    {"ep_re", "re_b", "re_bp", SP_RE}};
        draw_deva(2, true);
        for (int i = 0; i < 4; i++)
            if (!strcmp(s->voice, WHO[i].voice))
                stand(talking(WHO[i].sp) ? WHO[i].b : WHO[i].a, 238, 216, 2, 0);
        if (R.t % 60 == 40)
            fx_hearts(176, 110, 1);
        break;
    }
    case CAST_EP_MAGO: /* "sei una vera maga!": her wand shines */
        draw_deva(2, true);
        stand(talking(SP_MAGO) ? "mago_p" : "mago", 238, 216, 2, 0);
        if (R.t > 60 && R.t % 6 == 0) {
            int hx, hy;
            hero_hand(&R.deva, 2, &hx, &hy);
            int dx, dy;
            rng_pair(-10, 10, -10, 10, &dx, &dy);
            fx_twinkle(hx + dx, hy - 40 + dy, 0);
        }
        if (R.t == 70)
            fx_burst(72, 110, 16);
        break;
    case CAST_EP_PARTY: { /* everybody, at night under the fireworks */
        static const char *const ALL[16] = {"ciuffone", "polpone", "caramellone", "robottone", "melmoso",
                                            "lumacone", "fungone", "saltamolla", "rocciolo", "nevone",
                                            "ranocchione", "dinozzo", "tuonello", "fumino", "trombone",
                                            "trottolina"};
        char name[32];
        for (int row = 0; row < 2; row++) /* the friends of the four adventures, on two steps at the back */
            for (int i = 0; i < 8; i++) {
                snprintf(name, sizeof(name), "mo_%s_b", ALL[row * 8 + i]);
                party_hop(name, 10 + i * 40 + row * 20, 132 + row * 30, row * 8 + i, 60);
            }
        static const char *const FRONT[5][2] = {{"mago", "mago_p"}, {"strega_b", "strega_bp"}, {"stregone_b", "stregone_bp"},
                                                 {"orco_b", "orco_bp"}, {"re_b", "re_bp"}};
        static const int FX[5] = {28, 84, 236, 290, 126};
        for (int i = 0; i < 5; i++)
            party_hop((G.frame / (18 + i * 2)) & 1 ? FRONT[i][1] : FRONT[i][0], FX[i], 228, 20 + i, 80);
        R.deva.x = 180;
        R.deva.y = 230;
        hero_draw(&R.deva, &G.prog);
        int hx, hy;
        hero_hand(&R.deva, 1, &hx, &hy);
        gfx_blit(gfx_sprite("bacchetta"), hx - 4, hy - 18, 0);
        for (int i = 0; i < MAX_FLY; i++) /* the rockets going up */
            if (R.fly_t[i] >= 0 && R.fly_t[i] < ROCKET) {
                int y = 190 + (R.fly_y[i] - 190) * R.fly_t[i] / ROCKET;
                const sprite_t *tw = gfx_sprite(R.fly_t[i] & 2 ? "tw_2" : "tw_1");
                gfx_blit(tw, R.fly_x[i] - tw->w / 2, y, 0);
            }
        if (s->act == ACT_NONE && R.t % 45 == 0) { /* the end: they go on by themselves */
            int x, y;
            rng_pair(40, 280, 26, 86, &x, &y);
            fx_burst(x, y, 14);
        }
        break;
    }
    case CAST_PARTY4: { /* a party at the merry-go-round: the king lends his toys to everybody */
        static const char *const BACK[4] = {"mo_robottone_b", "mo_saltamolla_b", "mo_dinozzo_b", "mo_trottolina_b"};
        party_hop((G.frame / 24) & 1 ? "stregone_b" : "stregone_bp", 22, 222, 30, 100);
        party_hop((G.frame / 20) & 1 ? "orco_b" : "orco_bp", 300, 222, 31, 110);
        stand("strega_volo_b", 160, 70 - bob(16, 3), 1, 0);
        draw_friends_party(BACK, "re_b", "re_bp", 222);
        break;
    }
    }
}

/* the place: as it is, under the spell (grey, dark), or the colours coming back from the friend set
   free; in the dark tower, the colours come back in the light of her wand and of the lantern */
static void draw_place(const rstep_t *s)
{
    const arc_t *a = story_arc_def();
    const char *bg = R.bg ? R.bg : a->map;
    int r = s->look == LOOK_REVEAL ? R.reveal_t * 5 : 0;
    if (s->act == ACT_LANTERN)
        r = (int)R.light_r;
    if (s->look == LOOK_PLAIN || !R.bg || r > 420) {
        amb_bg(bg);
        return;
    }
    gfx_draw_bg(gfx_image_spell(bg, story_arc()));
    if (r > 0) {
        int cx = 238, cy = 140;
        if (s->act == ACT_LANTERN)
            lantern_pos(&cx, &cy);
        gfx_blit_image_circle(gfx_image(bg), cx, cy, r);
    }
}

/* her turn: the red button (the cross, for the dance) blinks by her */
static void draw_prompt(const rstep_t *s)
{
    if (!s->act || !R.act_ready || R.act_done_t >= 0 || R.act_n > 0 || ((R.t / 20) & 1))
        return;
    if (s->act == ACT_BALL && R.ball != BALL_DEVA)
        return;
    int hx, hy;
    hero_hand(&R.deva, s->act == ACT_FIREWORKS ? 1 : 2, &hx, &hy);
    if (s->act == ACT_DANCE) {
        static const char *const D[4] = {"dpad_up", "dpad_right", "dpad_down", "dpad_left"};
        gfx_blit(gfx_sprite(D[(R.t / 40) % 4]), 8, 40, 0);
    } else {
        gfx_blit(gfx_sprite("btn_a"), hx + 14, hy - 70, 0);
    }
}

static void draw(void)
{
    const rstep_t *s = cur();
    const arc_t *a = story_arc_def();
    draw_place(s);
    if (s->fx == FX_GREY) { /* the spell spreads from the villain (from the ogre's house, the king's castle) */
        int mx[6], my[6];
        map_points(mx, my);
        bool ogre = story_arc() == ARC_MUSICA || story_arc() == ARC_GIOCHI;
        gfx_blit_image_circle(gfx_image(a->map_off), ogre ? mx[CH_COUNT] : 160, ogre ? my[CH_COUNT] - 20 : 90, R.t * 4);
    }
    if (s->fx == FX_LIGHT) { /* ...and goes away, from the last place freed */
        const image_t *lit = gfx_image(a->map);
        int x, y, r;
        for (int i = 1; i < CH_COUNT; i++) { /* the places freed before stay as they were */
            region(i, &x, &y, &r);
            gfx_blit_image_circle(lit, x, y, r);
        }
        region(CH_COUNT, &x, &y, &r);
        gfx_blit_image_circle(lit, x, y, R.t * 3);
    }
    cast_draw(s);
    fx_draw();
    if (s->act == ACT_LANTERN && R.light_r < 400) { /* the dark of the tower, all around the light */
        int x, y;
        lantern_pos(&x, &y);
        gfx_fade_outside_circle(x, y, (int)R.light_r + 6, rgb565(0x08, 0x0a, 0x22), 150);
    }
    draw_prompt(s);
    /* the witch and the wizard come with thunder (the ogre just stamps his feet) */
    bool flash = s->fx == FX_LIGHTNING ||
                 (s->fx == FX_SHAKE && story_villain(R.ch) && story_arc() != ARC_MUSICA && story_arc() != ARC_GIOCHI);
    if (flash && (R.t < 4 || (R.t >= 8 && R.t < 10)))
        gfx_fade(rgb565(0xff, 0xff, 0xff), 170);
    if (R.shake > 0)
        gfx_shake(((R.shake / 2) & 1) ? 3 : -3, 0);
}

const scene_t SCENE_RACCONTO = {enter, update, draw};
