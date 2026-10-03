/* Deva's Awesome Adventures - "Le lettere" (0.10.0): the first letter of a word.
 *
 * In the library of the Mago Pistacchio a picture appears and the voice says
 * its word ("Mela! Con che lettera comincia?"); she picks the letter among
 * three big letter cards. The other way round, a big letter appears and she
 * picks the picture whose word starts with it. When the answer is right the
 * word is written under the picture, its first letter in pink: the first
 * step from the sound to the written word, before she can read.
 *
 * Levels: 1 the five vowels (ape, elefante, isola, ombrello, uva/uovo);
 * 2 vowels and the consonants that can be sung long (mmm, sss, lll, fff, nnn,
 * rrr, vvv); 3 every letter; 4 the letter is shown, pick the picture;
 * 5 both, with letters that look alike among the cards (M/N, E/F, B/D/P...).
 * Guided help: the word split in syllables ("Me... la."), then "la prima
 * lettera è... Emme!" and the right card lights up.
 * The words and their pictures are those of "Parole" (voices w_<word>,
 * syllables s_<word>), the letter names those of "Il mio nome" (lt_<letter>).
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "quiz.h"

typedef struct {
    const char *w; /* picture pic_<w>, voices w_<w>, s_<w> */
    char first;    /* its first letter, upper case */
} lword_t;

static const lword_t WORDS[] = {
    {"ape", 'A'},       {"elefante", 'E'},  {"isola", 'I'},     {"ombrello", 'O'},   {"uva", 'U'},
    {"uovo", 'U'},      {"banana", 'B'},    {"casa", 'C'},      {"cane", 'C'},       {"cuore", 'C'},
    {"corona", 'C'},    {"candela", 'C'},   {"campana", 'C'},   {"caramella", 'C'},  {"diamante", 'D'},
    {"farfalla", 'F'},  {"fiore", 'F'},     {"fiocco", 'F'},    {"gatto", 'G'},      {"gelato", 'G'},
    {"luna", 'L'},      {"letto", 'L'},     {"mela", 'M'},      {"microfono", 'M'},  {"nave", 'N'},
    {"palla", 'P'},     {"pane", 'P'},      {"pesce", 'P'},     {"porta", 'P'},      {"piatto", 'P'},
    {"pulcino", 'P'},   {"rana", 'R'},      {"rossetto", 'R'},  {"sole", 'S'},       {"stella", 'S'},
    {"scarpetta", 'S'}, {"torta", 'T'},     {"vela", 'V'},
};
#define NWORDS ARRAY_LEN(WORDS)

static const char VOWELS[] = "AEIOU";
static const char LONG_SOUNDS[] = "AEIOUMSLFNRV"; /* level 2: vowels and consonants that can be sung */
static const char LETTERS[] = "ABCDEFGILMNOPRSTUV"; /* every first letter of the words above */
static const char *LOOKALIKE[] = {"MN", "EF", "BDPR", "COG", "ILT", "UV", "AV"};

enum { M_LETTERA, M_FIGURA }; /* picture -> letter, letter -> picture */

#define HISTORY 5

static struct {
    int mode;
    int word;          /* the word of the question (M_LETTERA), or the right picture (M_FIGURA) */
    char letter;       /* its first letter */
    char opt[3];       /* M_LETTERA: letters on the cards */
    int pic[3];        /* M_FIGURA: words on the cards */
    int history[HISTORY];
    bool similar;      /* look-alike letters among the cards */
} L;

static const char *lookalike_group(char c)
{
    for (int i = 0; i < ARRAY_LEN(LOOKALIKE); i++)
        if (strchr(LOOKALIKE[i], c))
            return LOOKALIKE[i];
    return "";
}

static void say_letter(char c, bool queue)
{
    char id[8];
    snprintf(id, sizeof(id), "lt_%c", c - 'A' + 'a');
    if (queue)
        say_then(id);
    else
        say(id);
}

static void say_word(int w, const char *prefix, bool queue)
{
    char id[32];
    snprintf(id, sizeof(id), "%s%s", prefix, WORDS[w].w);
    if (queue)
        say_then(id);
    else
        say(id);
}

static bool recent(int w)
{
    for (int i = 0; i < HISTORY; i++)
        if (L.history[i] == w)
            return true;
    return false;
}

static void remember(int w)
{
    memmove(&L.history[1], &L.history[0], sizeof(int) * (HISTORY - 1));
    L.history[0] = w;
}

/* a word whose first letter is in the set, not asked lately */
static int pick_word(const char *set)
{
    int pool[NWORDS], n = 0;
    for (int pass = 0; pass < 2 && n == 0; pass++)
        for (int i = 0; i < NWORDS; i++)
            if (strchr(set, WORDS[i].first) && (pass == 1 || !recent(i)))
                pool[n++] = i;
    /* every letter as likely as the others (P has many words, V one) */
    char letters[32];
    int nl = 0;
    for (int i = 0; i < n; i++)
        if (!memchr(letters, WORDS[pool[i]].first, (size_t)nl))
            letters[nl++] = WORDS[pool[i]].first;
    char c = letters[rng_range(0, nl - 1)];
    int same[NWORDS], ns = 0;
    for (int i = 0; i < n; i++)
        if (WORDS[pool[i]].first == c)
            same[ns++] = pool[i];
    return same[rng_range(0, ns - 1)];
}

/* two other letters for the cards: from the level's set; look-alike ones when asked */
static void make_letters(quiz_t *q, const char *set)
{
    const char *group = lookalike_group(L.letter);
    char d[2];
    for (int k = 0; k < 2; k++) {
        for (int tries = 0;; tries++) {
            char c;
            if (L.similar && k == 0 && strlen(group) > 1 && tries < 40)
                c = group[rng_range(0, (int)strlen(group) - 1)];
            else
                c = set[rng_range(0, (int)strlen(set) - 1)];
            bool bad = c == L.letter || (k == 1 && c == d[0]);
            if (!bad && (!L.similar || k == 1) && strchr(group, c) && tries < 200)
                bad = true; /* easy cards: no look-alikes */
            if (!bad) {
                d[k] = c;
                break;
            }
        }
    }
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        L.opt[i] = i == q->correct ? L.letter : d[k++];
}

/* two other pictures whose words start with other letters */
static void make_pictures(quiz_t *q)
{
    const char *group = lookalike_group(L.letter);
    int d[2];
    for (int k = 0; k < 2; k++) {
        for (int tries = 0;; tries++) {
            int w = rng_range(0, NWORDS - 1);
            char c = WORDS[w].first;
            bool bad = c == L.letter || (k == 1 && c == WORDS[d[0]].first);
            if (!bad && L.similar && k == 0 && strlen(group) > 1 && !strchr(group, c) && tries < 60)
                bad = true; /* a picture starting with a look-alike letter */
            if (!bad) {
                d[k] = w;
                break;
            }
        }
    }
    q->correct = rng_range(0, 2);
    for (int i = 0, k = 0; i < 3; i++)
        L.pic[i] = i == q->correct ? L.word : d[k++];
}

static void setup(quiz_t *q, int level)
{
    if (q->question == 0)
        for (int i = 0; i < HISTORY; i++)
            L.history[i] = -1;
    const char *set = level == 1 ? VOWELS : (level == 2 ? LONG_SOUNDS : LETTERS);
    L.mode = level <= 3 ? M_LETTERA : (level == 4 ? M_FIGURA : (rng_range(0, 1) ? M_FIGURA : M_LETTERA));
    L.similar = level >= 5;
    L.word = pick_word(set);
    L.letter = WORDS[L.word].first;
    remember(L.word);
    if (L.mode == M_LETTERA) {
        q->card = QUIZ_CARD_NUMBER;
        make_letters(q, set);
    } else {
        q->card = QUIZ_CARD_PICTURE;
        make_pictures(q);
    }
}

static bool prepare(quiz_t *q)
{
    if (q->t == 1)
        sfx("pop");
    return q->t > 20;
}

static void ask(quiz_t *q)
{
    if (L.mode == M_LETTERA) { /* "Mela! Con che lettera comincia?" */
        say_word(L.word, "w_", false);
        say_then("let_comincia");
    } else { /* "Quale parola inizia con la lettera... Emme" */
        say("par_lettera");
        say_letter(L.letter, true);
    }
}

static void hover(quiz_t *q, int i)
{
    if (L.mode == M_LETTERA)
        say_letter(L.opt[i], false);
    else
        say_word(L.pic[i], "w_", false);
}

/* "Mela comincia con la... Emme!" */
static void right(quiz_t *q)
{
    say_word(L.word, "w_", true);
    say_then("let_con_la");
    say_letter(L.letter, true);
}

static bool hint(quiz_t *q)
{
    if (q->hint_step == 0) { /* the word in syllables: "Me... la." */
        say_word(L.word, "s_", false);
        return false;
    }
    say("let_prima"); /* "La prima lettera è..." */
    say_letter(L.letter, true);
    return true;
}

/* the word written under the picture, its first letter in pink */
static void draw_word(int cx, int y)
{
    char up[16];
    snprintf(up, sizeof(up), "%s", WORDS[L.word].w);
    for (char *c = up; *c; c++)
        *c = (char)(*c - 'a' + 'A');
    int w = gfx_text_big_width(up, 1), x = cx - w / 2;
    char first[2] = {up[0], 0};
    int fw = gfx_text_big_width(first, 1);
    gfx_text_big(first, x + fw / 2, y, NUM_SELECTED, 1);
    if (up[1])
        gfx_text_big(up + 1, x + fw + gfx_text_big_width(up + 1, 1) / 2 + 1, y, NUM_NORMAL, 1);
}

static void draw(quiz_t *q)
{
    int cx = q->panel_cx, cy = q->panel_cy;
    bool solved = q->state == Q_RIGHT;
    int bob = swing((int)G.frame, 60, 1);
    if (L.mode == M_LETTERA) {
        char name[32];
        snprintf(name, sizeof(name), "pic_%s", WORDS[L.word].w);
        const sprite_t *s = gfx_sprite(name);
        int k = q->state == Q_PREPARE && q->t < 6 ? 1 : 2, py = solved ? cy - 22 : cy - 4;
        gfx_blit_scaled(s, cx - s->w * k / 2, py - s->h * k / 2 + bob, k, 0);
        if (solved)
            draw_word(cx, cy + 42);
    } else {
        char up[2] = {L.letter, 0};
        int ly = solved ? cy - 26 : cy - 4, lx = solved ? cx - 28 : cx;
        gfx_text_big(up, lx, ly + bob, NUM_SELECTED, 3);
        if (solved) { /* the picture next to its letter, and the word */
            char name[32];
            snprintf(name, sizeof(name), "pic_%s", WORDS[L.word].w);
            const sprite_t *s = gfx_sprite(name);
            gfx_blit_scaled(s, cx + 34 - s->w, ly - s->h, 2, 0);
            draw_word(cx, cy + 42);
        }
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (L.mode == M_LETTERA) {
        char s[2] = {L.opt[i], 0};
        gfx_text_big(s, cx, cy, style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL), 2);
        return;
    }
    char name[32];
    snprintf(name, sizeof(name), "pic_%s", WORDS[L.pic[i]].w);
    const sprite_t *s = gfx_sprite(name);
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    if (L.mode == M_LETTERA)
        snprintf(buf, n, "%c", L.opt[i]);
    else
        snprintf(buf, n, "%s", WORDS[L.pic[i]].w);
}

static const char *subject(quiz_t *q)
{
    static char s[40];
    snprintf(s, sizeof(s), "%s:%s:%c", L.mode == M_LETTERA ? "lettera" : "figura", WORDS[L.word].w, L.letter);
    return s;
}

const quiz_def_t QUIZ_LETTERE = {
    .name = "lettere",
    .game = GAME_LETTERE,
    .bg = "bg_biblioteca",
    .intro = "let_intro",
    .card = QUIZ_CARD_NUMBER,
    .setup = setup,
    .prepare = prepare,
    .ask = ask,
    .hover = hover,
    .right = right,
    .hint = hint,
    .draw = draw,
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void) { quiz_enter(&QUIZ_LETTERE); }

const scene_t SCENE_LETTERE = {enter, quiz_update, quiz_draw};
