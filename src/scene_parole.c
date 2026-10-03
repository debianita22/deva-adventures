/* Deva's Awesome Adventures - "Parole": listening games that lead to reading.
 *
 * Five kinds of question, unlocked by level (easiest first):
 *   fusione  "Ascolta bene... Stel... la. Che cosa ho detto?"  -> pick the picture
 *   quanti   "Quanti pezzetti ha questa parola? Stel... la."   -> pick 1..5 (claps)
 *   rima     "Quale parola fa rima con... Fiore?"               -> cuore
 *   inizia   "Quale parola inizia con... ME"                    -> pick the picture
 *   lettera  "Quale parola inizia con la lettera... A"          -> pick the picture
 * Words are spoken split into syllables; the syllable onsets come from
 * voce/sillabe.txt, so the beat blocks light up and Deva steps exactly on
 * each syllable she hears.
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

enum { M_FUSIONE, M_QUANTI, M_INIZIA, M_LETTERA, M_RIMA, M_COUNT };
static const char *MODE_NAMES[M_COUNT] = {"fusione", "quanti", "inizia", "lettera", "rima"};

#define MAX_SYL 5
#define HISTORY 4 /* words not repeated within this many questions */

typedef struct {
    const char *w;
    int syl;             /* syllables (fallback when sillabe.txt is missing) */
    const char *first;   /* first syllable with a voice clip "sy_<first>", or NULL */
    const char *rima;    /* ending from the stressed vowel: words with the same one rhyme */
} word_t;

static const word_t WORDS[] = {
    {"stella", 2, NULL, "ella"},    {"cuore", 2, NULL, "ore"},       {"farfalla", 3, NULL, "alla"},
    {"fiore", 2, NULL, "ore"},      {"diamante", 3, NULL, "ante"},   {"corona", 3, "co", "ona"},
    {"scarpetta", 3, NULL, "etta"}, {"rossetto", 3, NULL, "etto"},   {"fiocco", 2, NULL, "occo"},
    {"microfono", 4, "mi", "ofono"}, {"pennello", 3, NULL, "ello"},  {"smalto", 2, NULL, "alto"},
    {"palla", 2, NULL, "alla"},     {"mela", 2, "me", "ela"},        {"luna", 2, "lu", "una"},
    {"sole", 2, "so", "ole"},       {"pesce", 2, "pe", "esce"},      {"casa", 2, "ca", "asa"},
    {"torta", 2, NULL, "orta"},     {"banana", 3, "ba", "ana"},      {"gelato", 3, "ge", "ato"},
    {"palloncino", 4, NULL, "ino"}, {"ape", 2, NULL, "ape"},         {"uva", 2, NULL, "uva"},
    {"uovo", 2, NULL, "ovo"},       {"ombrello", 3, NULL, "ello"},   {"isola", 3, NULL, "isola"},
    {"elefante", 4, NULL, "ante"},
    /* rhyme partners (also used by the other questions) */
    {"cane", 2, "ca", "ane"},       {"pane", 2, NULL, "ane"},        {"gatto", 2, NULL, "atto"},
    {"piatto", 2, NULL, "atto"},    {"nave", 2, NULL, "ave"},        {"chiave", 2, NULL, "ave"},
    {"porta", 2, NULL, "orta"},     {"vela", 2, NULL, "ela"},        {"candela", 3, NULL, "ela"},
    {"padella", 3, NULL, "ella"},   {"caramella", 4, "ca", "ella"},  {"letto", 2, NULL, "etto"},
    {"rana", 2, NULL, "ana"},       {"campana", 3, NULL, "ana"},     {"pulcino", 3, NULL, "ino"},
};
#define NWORDS ARRAY_LEN(WORDS)

static struct {
    bool timing_loaded;
    int nsyl[NWORDS];
    int onset[NWORDS][MAX_SYL]; /* ms from the start of "s_<word>" */

    int mode;
    int history[HISTORY];       /* recent answers, -1 = empty */
    int word;                   /* the word of the question */
    int pic[3];                 /* picture cards: word index */
    int answer;                 /* rima: the word that rhymes */
    int num[3];                 /* number cards: syllable counts */
    char letters[4];            /* inizia: "ME", lettera: "A" */

    sound_t s_clip;             /* "s_<word>" */
    sound_t cue_clip;           /* "sy_<syllable>" / "l_<vowel>" */
    int cur_syl;                /* syllable being spoken, -1 = none */
    int lit;                    /* blocks lit in the current/last playback */
    bool show_blocks, clap;
    int step_flip;
} P;

/* ------------------------------------------------------------------ data */

static int word_index(const char *w)
{
    for (int i = 0; i < NWORDS; i++)
        if (!strcmp(WORDS[i].w, w))
            return i;
    return -1;
}

static void load_timing(void)
{
    P.timing_loaded = true;
    for (int i = 0; i < NWORDS; i++) {
        P.nsyl[i] = 0; /* 0 = unknown: fall back to an even split */
    }
    char path[600];
    snprintf(path, sizeof(path), "%s/voce/sillabe.txt", dir_data());
    FILE *f = fopen(path, "r");
    if (!f) {
        log_msg(LOG_WARN, "no %s: syllable beats will be approximate\n", path);
        return;
    }
    char line[256], id[48];
    while (fgets(line, sizeof(line), f)) {
        int n, o[MAX_SYL] = {0};
        if (line[0] == '#')
            continue;
        int k = sscanf(line, "s_%47s %d %d %d %d %d %d", id, &n, &o[0], &o[1], &o[2], &o[3], &o[4]);
        int w = k >= 3 ? word_index(id) : -1;
        if (w < 0 || n < 1 || n > MAX_SYL || k < 2 + n)
            continue;
        P.nsyl[w] = n;
        memcpy(P.onset[w], o, sizeof(o));
    }
    fclose(f);
}

static int syllables(int w) { return P.nsyl[w] > 0 ? P.nsyl[w] : WORDS[w].syl; }

static int onset_ms(int w, int i)
{
    if (P.nsyl[w] > 0)
        return P.onset[w][i];
    int len = snd_length_frames(P.s_clip) * 1000 / FPS; /* even split */
    return len * i / (WORDS[w].syl > 0 ? WORDS[w].syl : 1);
}

static char first_letter(int w) { return (char)(WORDS[w].w[0] - 'a' + 'A'); }

/* near rhymes count as rhymes when choosing distractors: double consonants
 * are collapsed, so stella/mela and gatto/gelato never appear together */
static bool rhyme_near(int a, int b)
{
    char ka[16], kb[16];
    const char *src[2] = {WORDS[a].rima, WORDS[b].rima};
    char *dst[2] = {ka, kb};
    for (int k = 0; k < 2; k++) {
        int n = 0;
        for (const char *c = src[k]; *c && n < 15; c++)
            if (n == 0 || dst[k][n - 1] != *c)
                dst[k][n++] = *c;
        dst[k][n] = 0;
    }
    return !strcmp(ka, kb);
}

static bool has_rhyme(int w)
{
    for (int i = 0; i < NWORDS; i++)
        if (i != w && !strcmp(WORDS[i].rima, WORDS[w].rima))
            return true;
    return false;
}

static void say_word(const char *prefix, int w, bool queue)
{
    char id[48];
    snprintf(id, sizeof(id), "%s_%s", prefix, WORDS[w].w);
    if (queue)
        say_then(id);
    else
        say(id);
}

/* ------------------------------------------------------------------ question */

static int pick_mode(int level)
{
    int r = rng_range(0, 99);
    switch (level) {
    /* one new kind of question per level, the ones already known stay in the mix */
    case 1: return M_FUSIONE;
    case 2: return r < 60 ? M_FUSIONE : M_QUANTI;
    case 3: return r < 25 ? M_FUSIONE : (r < 55 ? M_QUANTI : M_RIMA);
    case 4: return r < 25 ? M_QUANTI : (r < 60 ? M_RIMA : M_INIZIA);
    default: return r < 10 ? M_FUSIONE : r < 25 ? M_QUANTI : r < 50 ? M_RIMA : r < 75 ? M_INIZIA : M_LETTERA;
    }
}

static bool target_ok(int mode, int level, int w)
{
    switch (mode) {
    case M_FUSIONE: return level > 1 || syllables(w) == 2; /* short words first */
    case M_INIZIA: return WORDS[w].first != NULL;
    case M_LETTERA: return strchr("aeiou", WORDS[w].w[0]) != NULL;
    case M_RIMA: return has_rhyme(w);
    default: return true;
    }
}

/* Distractor pictures start with a different letter than the answer and
 * than each other; only in "fusione" from level 3 on may they sound alike
 * (pal-la vs pal-lon-ci-no). 16 different first letters: always satisfiable. */
static bool distractor_ok(int mode, int level, int w, const int *taken, int n)
{
    for (int i = 0; i < n; i++) {
        if (taken[i] == w)
            return false;
        if (WORDS[taken[i]].w[0] == WORDS[w].w[0] && !(mode == M_FUSIONE && level >= 3))
            return false;
    }
    return !(mode == M_FUSIONE && level == 1 && syllables(w) != 2);
}

static void setup(quiz_t *q, int level)
{
    if (!P.timing_loaded)
        load_timing();
    if (q->question == 0)
        memset(P.history, -1, sizeof(P.history));
    P.mode = pick_mode(level);
    /* a word not heard recently; in the letter games also a new first letter
     * (uva and uovo both start with U) - relaxed if nothing is left */
    for (int tries = 0;; tries++) {
        P.word = rng_range(0, NWORDS - 1);
        if (!target_ok(P.mode, level, P.word))
            continue;
        bool recent = false;
        for (int i = 0; i < HISTORY && tries < 200; i++) {
            int h = P.history[i];
            if (h == P.word || (h >= 0 && P.mode >= M_INIZIA && WORDS[h].w[0] == WORDS[P.word].w[0]))
                recent = true;
        }
        if (!recent || tries >= 400)
            break;
    }
    memmove(P.history + 1, P.history, (HISTORY - 1) * sizeof(P.history[0]));
    P.history[0] = P.word;

    char id[48];
    snprintf(id, sizeof(id), "s_%s", WORDS[P.word].w);
    P.s_clip = snd_find(SND_VOICE, id);
    P.cur_syl = -1;
    P.lit = 0;
    P.clap = false;
    P.show_blocks = P.mode == M_FUSIONE;
    P.letters[0] = 0;
    P.cue_clip = -1;
    if (P.mode == M_INIZIA) {
        snprintf(P.letters, sizeof(P.letters), "%s", WORDS[P.word].first);
        for (char *c = P.letters; *c; c++)
            *c = (char)(*c - 'a' + 'A');
        snprintf(id, sizeof(id), "sy_%s", WORDS[P.word].first);
        P.cue_clip = snd_find(SND_VOICE, id);
    } else if (P.mode == M_LETTERA) {
        P.letters[0] = first_letter(P.word);
        P.letters[1] = 0;
        snprintf(id, sizeof(id), "l_%c", WORDS[P.word].w[0]);
        P.cue_clip = snd_find(SND_VOICE, id);
    }

    if (P.mode == M_RIMA) {
        /* the answer rhymes exactly; the others do not rhyme, not even nearly;
         * from level 4 one of them starts like the word (fiore / fiocco) */
        q->card = QUIZ_CARD_PICTURE;
        int cand[NWORDS], nc = 0;
        for (int i = 0; i < NWORDS; i++)
            if (i != P.word && !strcmp(WORDS[i].rima, WORDS[P.word].rima))
                cand[nc++] = i;
        P.answer = cand[rng_range(0, nc - 1)];
        int d[2], nd = 0;
        if (level >= 4) {
            int trap[NWORDS], nt = 0;
            for (int i = 0; i < NWORDS; i++)
                if (WORDS[i].w[0] == WORDS[P.word].w[0] && i != P.word && !rhyme_near(i, P.word))
                    trap[nt++] = i;
            if (nt)
                d[nd++] = trap[rng_range(0, nt - 1)];
        }
        while (nd < 2) {
            int w = rng_range(0, NWORDS - 1);
            if (w != P.word && w != P.answer && (nd == 0 || w != d[0]) && !rhyme_near(w, P.word) &&
                !rhyme_near(w, P.answer))
                d[nd++] = w;
        }
        q->correct = rng_range(0, 2);
        int k = 0;
        for (int i = 0; i < 3; i++)
            P.pic[i] = i == q->correct ? P.answer : d[k++];
    } else if (P.mode == M_QUANTI) {
        q->card = QUIZ_CARD_NUMBER;
        int n = syllables(P.word), lo;
        do
            lo = n - rng_range(0, 2);
        while (lo < 1 || lo + 2 > 5);
        for (int i = 0; i < 3; i++)
            P.num[i] = lo + i;
        q->correct = n - lo;
    } else {
        q->card = QUIZ_CARD_PICTURE;
        int taken[3] = {P.word, -1, -1}, n = 1;
        while (n < 3) {
            int w = rng_range(0, NWORDS - 1);
            if (distractor_ok(P.mode, level, w, taken, n))
                taken[n++] = w;
        }
        q->correct = rng_range(0, 2);
        int k = 1;
        for (int i = 0; i < 3; i++)
            P.pic[i] = i == q->correct ? taken[0] : taken[k++];
    }
}

/* ------------------------------------------------------------------ beats */

/* Follow the voice: which syllable of "s_<word>" is being spoken right now.
 * On every new syllable: light a block, Deva takes a dance step (and claps
 * during the guided help of "quanti"). */
static void tick(quiz_t *q)
{
    int syl = -1;
    if (P.s_clip >= 0 && voice_current() == P.s_clip) {
        int ms = voice_pos_ms();
        for (int i = 0; i < syllables(P.word); i++)
            if (ms >= onset_ms(P.word, i))
                syl = i;
    }
    if (syl != P.cur_syl && syl >= 0) {
        P.lit = syl + 1; /* blocks heard so far in this playback */
        P.step_flip = !P.step_flip;
        if (q->state != Q_RIGHT)
            hero_move(&G.hero, (syl & 1) ? POSE_UP : POSE_STEP, P.step_flip, 16);
        if (P.clap)
            sfx("clap");
    }
    P.cur_syl = syl;
}

/* ------------------------------------------------------------------ flow */

static bool prepare(quiz_t *q)
{
    if (P.mode == M_FUSIONE) {
        if (q->t == 1)
            say("par_ascolta");
        return q->t > 10 && !voice_busy();
    }
    if (q->t == 1)
        sfx("pop");
    return q->t > 24;
}

static void ask(quiz_t *q)
{
    char id[16];
    switch (P.mode) {
    case M_FUSIONE:
        say_word("s", P.word, false);
        say_then("par_cosa");
        break;
    case M_QUANTI:
        say("par_quanti");
        say_word("s", P.word, true);
        break;
    case M_INIZIA:
        say("par_inizia");
        snprintf(id, sizeof(id), "sy_%s", WORDS[P.word].first);
        say_then(id);
        break;
    case M_LETTERA:
        say("par_lettera");
        snprintf(id, sizeof(id), "l_%c", WORDS[P.word].w[0]);
        say_then(id);
        break;
    case M_RIMA:
        say("rima_q");
        say_word("w", P.word, true);
        break;
    }
}

static void hover(quiz_t *q, int i)
{
    if (q->card == QUIZ_CARD_NUMBER) {
        char id[8];
        snprintf(id, sizeof(id), "n%02d", P.num[i]);
        say(id);
    } else {
        say_word("w", P.pic[i], false);
    }
}

static void right(quiz_t *q)
{
    /* hear it once more, whole or in beats (clapping the beats in "quanti") */
    if (P.mode == M_QUANTI || P.mode == M_INIZIA) {
        P.show_blocks = true;
        P.clap = P.mode == M_QUANTI;
        say_word("s", P.word, true);
    } else if (P.mode == M_RIMA) { /* "Fiore... cuore: fanno rima!" */
        say_word("w", P.word, true);
        say_word("w", P.answer, true);
        say_then("rima_si");
    } else {
        say_word("w", P.word, true);
    }
}

static bool hint(quiz_t *q)
{
    char id[16];
    switch (P.mode) {
    case M_FUSIONE: /* the beats again, then the whole word */
        if (q->hint_step == 0)
            say_word("s", P.word, false);
        else if (q->hint_step == 1)
            say_word("w", P.word, false);
        else
            break;
        return false;
    case M_QUANTI: /* clap every beat together, blocks on */
        if (q->hint_step == 0) {
            say("par_battiamo");
            P.show_blocks = true;
            P.clap = true;
            return false;
        }
        if (q->hint_step == 1) {
            say_word("s", P.word, false);
            return false;
        }
        P.clap = false;
        say("aiuto_scegli");
        return true;
    case M_INIZIA: /* the syllable, then the word in beats */
        if (q->hint_step == 0) {
            snprintf(id, sizeof(id), "sy_%s", WORDS[P.word].first);
            say(id);
            return false;
        }
        if (q->hint_step == 1) {
            P.show_blocks = true;
            say_word("s", P.word, false);
            return false;
        }
        break;
    case M_LETTERA: /* the letter, then the word */
        if (q->hint_step == 0) {
            snprintf(id, sizeof(id), "l_%c", WORDS[P.word].w[0]);
            say(id);
            return false;
        }
        if (q->hint_step == 1) {
            say_word("w", P.word, false);
            return false;
        }
        break;
    case M_RIMA: /* "Ascolta come finiscono: fiore... cuore" */
        if (q->hint_step == 0) {
            say("rima_aiuto");
            return false;
        }
        if (q->hint_step == 1) {
            say_word("w", P.word, false);
            return false;
        }
        if (q->hint_step == 2) {
            say_word("w", P.answer, false);
            return false;
        }
        break;
    }
    say("par_scegli");
    return true;
}

/* ------------------------------------------------------------------ drawing */

/* the beats of the word, twice as big as the little blocks they are drawn from (0.12) */
static void draw_blocks(int cx, int y)
{
    int n = syllables(P.word), k = 2, step = 26 * k;
    int x0 = cx - (n * step - 6 * k) / 2;
    for (int i = 0; i < n; i++) {
        const char *name = "sblock_off";
        int dy = 0;
        if (i == P.cur_syl) {
            name = "sblock_cur";
            dy = -3;
        } else if (i < P.lit) {
            name = "sblock_on";
        }
        gfx_blit_scaled(gfx_sprite(name), x0 + i * step, y + dy, k, 0);
        if (P.clap && i == P.cur_syl)
            gfx_blit(gfx_sprite("hand"), x0 + i * step + 5 * k, y - 14, 0);
    }
}

/* the parrot on its branch (bg_pappagallo, 0.12): it says the words with the voice */
static void draw_parrot(void)
{
    bool beak = voice_busy() && ((G.frame / 6) & 1);
    gfx_blit_scaled(gfx_sprite(beak ? "pappagallo_p" : "pappagallo"), 4, 24, 2, 0);
}

static void intro_draw(quiz_t *q)
{
    if (!q->duel)
        draw_parrot();
}

static void draw_picture(int w, int cx, int cy, int k)
{
    char name[32];
    snprintf(name, sizeof(name), "pic_%s", WORDS[w].w);
    const sprite_t *s = gfx_sprite(name);
    gfx_blit_scaled(s, cx - s->w * k / 2, cy - s->h * k / 2, k, 0);
}

static void draw(quiz_t *q)
{
    int cx = q->panel_cx, pic_cy = q->panel_cy - 14, block_y = q->panel_cy + 30; /* 0.12: the beats at 2x */
    bool solved = q->state == Q_RIGHT;
    if (!q->duel)
        draw_parrot();
    int zoom = (q->state == Q_PREPARE && q->t < 6) ? 1 : 2; /* pops in */
    bool cue = P.cue_clip >= 0 && voice_current() == P.cue_clip;
    switch (P.mode) {
    case M_FUSIONE:
        if (solved) {
            draw_picture(P.word, cx, pic_cy, 2); /* the answer is revealed */
        } else {
            const sprite_t *qm = gfx_sprite("qmark");
            gfx_blit_scaled(qm, cx - qm->w, pic_cy - qm->h + swing((int)G.frame, 60, 1), 2, 0);
        }
        draw_blocks(cx, block_y);
        break;
    case M_QUANTI:
        draw_picture(P.word, cx, pic_cy, zoom);
        if (P.show_blocks || solved)
            draw_blocks(cx, block_y);
        break;
    case M_RIMA:
        if (solved) { /* the two words side by side */
            draw_picture(P.word, cx - 44, pic_cy + 6, 2);
            draw_picture(P.answer, cx + 44, pic_cy + 6, 2);
        } else {
            draw_picture(P.word, cx, pic_cy + 6, zoom);
        }
        break;
    case M_INIZIA:
    case M_LETTERA:
        /* the letters light up while the voice says them */
        gfx_text_big(P.letters, cx, pic_cy - (cue ? 2 : 0), cue ? NUM_SELECTED : NUM_NORMAL,
                     zoom == 1 ? 1 : (P.mode == M_LETTERA ? 3 : 2));
        if (P.show_blocks) /* inizia: the word in beats, during the help and after */
            draw_blocks(cx, block_y);
        break;
    }
}

static void card_draw(quiz_t *q, int i, int cx, int cy, int style)
{
    if (q->card == QUIZ_CARD_NUMBER) {
        int n = P.num[i];
        gfx_number_big(n, cx, cy - 7, style == CARD_OFF ? NUM_OFF : (style == CARD_SELECTED ? NUM_SELECTED : NUM_NORMAL));
        if (style == CARD_OFF)
            gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
        const sprite_t *hand = gfx_sprite("hand");
        int x0 = cx - (n * (hand->w + 1) - 1) / 2;
        for (int k = 0; k < n; k++)
            gfx_blit(hand, x0 + k * (hand->w + 1), cy + 9, 0);
        gfx_set_tint(0, 0);
        return;
    }
    if (style == CARD_OFF)
        gfx_set_tint(rgb565(0xb0, 0xa4, 0xc8), 200);
    draw_picture(P.pic[i], cx, cy, 1);
    gfx_set_tint(0, 0);
}

static void card_value(quiz_t *q, int i, char *buf, size_t n)
{
    if (q->card == QUIZ_CARD_NUMBER)
        snprintf(buf, n, "%d", P.num[i]);
    else
        snprintf(buf, n, "%s", WORDS[P.pic[i]].w);
}

static const char *subject(quiz_t *q)
{
    static char s[48];
    snprintf(s, sizeof(s), "%s:%s", MODE_NAMES[P.mode], WORDS[P.word].w);
    return s;
}

const quiz_def_t QUIZ_PAROLE = {
    .name = "parole",
    .game = GAME_PAROLE,
    .bg = "bg_pappagallo", /* 0.12: under the parrot's tree, the question in its speech bubble */
    .intro = "par_intro",
    .card = QUIZ_CARD_PICTURE,
    .setup = setup,
    .prepare = prepare,
    .tick = tick,
    .ask = ask,
    .hover = hover,
    .right = right,
    .hint = hint,
    .draw = draw,
    .intro_draw = intro_draw,
    .card_draw = card_draw,
    .card_value = card_value,
    .subject = subject,
};

static void enter(void) { quiz_enter(&QUIZ_PAROLE); }

const scene_t SCENE_PAROLE = {enter, quiz_update, quiz_draw};
