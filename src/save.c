/* Deva's Awesome Adventures - settings, profiles and parent log (plain text files).
 * SPDX-License-Identifier: MIT
 */
#include "save.h"
#include "plat.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

const char *const GAME_IDS[GAME_COUNT] = {"conta",    "parole",     "sequenze", "balla",  "nome",
                                          "memory",   "ritmo",      "dove",     "emozioni", "storie",
                                          "ginnastica", "trucco",   "forme",    "lettere", "ombre",
                                          "sentiero", "negozio",   "misure"};
const char *const GAME_TITLES[GAME_COUNT] = {"Conta",  "Parole", "Sequenze",  "Balla con me",  "Il mio nome",
                                             "Memory", "Ritmo",  "Sopra e sotto", "Emozioni", "Storie",
                                             "Ginnastica", "Trucca i mostri", "Forme e colori", "Le lettere",
                                             "Le ombre", "Il sentiero", "Il negozio", "Le misure"};
const char *const GAME_ICONS[GAME_COUNT] = {"menu_conta",  "menu_parole", "menu_sequenze", "menu_balla",
                                            "menu_nome",   "menu_memory", "menu_ritmo",    "menu_dove",
                                            "menu_emozioni", "menu_storie", "menu_ginnastica", "menu_trucco",
                                            "menu_forme",  "menu_lettere", "menu_ombre",   "menu_sentiero",
                                            "menu_negozio", "menu_misure"};
/* the order in which new games open: the gym and the make-up salon early, for her;
   the games of 0.10 and 0.11 among the others (a profile that has them all gets them one by one) */
static const int UNLOCK_ORDER[] = {GAME_NOME,  GAME_GINNASTICA, GAME_MEMORY,   GAME_TRUCCO, GAME_OMBRE,
                                   GAME_RITMO, GAME_LETTERE,    GAME_NEGOZIO,  GAME_FORME,  GAME_DOVE,
                                   GAME_SENTIERO, GAME_MISURE,  GAME_EMOZIONI, GAME_STORIE};

const item_t ITEMS[] = {
    {"ombretto_rosa", SLOT_EYES, "rosa"},     {"ombretto_azzurro", SLOT_EYES, "azzurro"},
    {"ombretto_viola", SLOT_EYES, "viola"},   {"ombretto_oro", SLOT_EYES, "oro"},
    {"rossetto_rosso", SLOT_LIPS, "rosso"},   {"rossetto_rosa", SLOT_LIPS, "rosa"},
    {"rossetto_viola", SLOT_LIPS, "viola"},   {"guance", SLOT_BLUSH, NULL},
    {"adesivo_stella", SLOT_STICKER, "stella"}, {"adesivo_cuore", SLOT_STICKER, "cuore"},
    {"brillantini", SLOT_GLITTER, NULL},      {"coroncina", SLOT_HEAD, "tiara"},
    {"fiore", SLOT_HEAD, "flower"},
};
const int ITEM_COUNT = (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));

const char *const STICKERS[STICKER_COUNT] = {
    "stella",  "cuore",   "farfalla", "fiore",    "diamante",  "corona",    "scarpetta", "rossetto", "fiocco",
    "microfono", "pennello", "smalto", "palla",   "mela",      "luna",      "sole",      "pesce",    "casa",
    "torta",   "banana",  "gelato",   "palloncino", "ape",     "uva",       "uovo",      "ombrello", "isola",
    "elefante", "cane",   "pane",     "gatto",    "piatto",    "nave",      "chiave",    "porta",    "vela",
    "candela", "caramella", "letto",  "rana",     "campana",   "pulcino",
};

static const char *SLOT_KEYS[6] = {"occhi", "labbra", "guance", "adesivo", "brillantini", "testa"};

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
        *--e = 0;
    return s;
}

/* Calls fn(key, value) for each "key = value" line; '#' starts a comment. */
static bool read_kv(const char *path, void (*fn)(void *ctx, const char *k, const char *v), void *ctx)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash)
            *hash = 0;
        char *eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = 0;
        fn(ctx, trim(line), trim(eq + 1));
    }
    fclose(f);
    return true;
}

/* ------------------------------------------------------------------ files that survive a power cut (0.15)
 *
 * A child switches the console off when she likes, and the saves often live on the FAT/exFAT card.
 * A file is written to "<path>.tmp" and closed with a line "# fine" (the file is whole), flushed to
 * the card (fsync); the version before becomes "<path>.prev", then the new one takes its name, and
 * the folder is flushed too. On loading, a file missing or without its "# fine" line (cut by a power
 * cut, or by a card that did not finish writing) gives way to the newest whole copy: the ".tmp" if
 * it got to its "# fine", else the ".prev". At most the last round is lost, never the profile.
 * A file of 0.14 or before has no "# fine" and no ".prev": it loads as it is. */
#define END_MARK "# fine"

static bool exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* The file has its END_MARK line and no hole (the zeros of a card that did not finish writing).
 * Line by line, so a file edited by hand still counts: lines added after it, CR-LF, no last newline. */
static bool file_whole(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char buf[4096], line[64];
    size_t n, len = 0;
    bool marked = false, hole = false;
    while (!hole && (n = fread(buf, 1, sizeof(buf), f)) > 0)
        for (size_t i = 0; i < n && !hole; i++) {
            if (buf[i] == 0) {
                hole = true;
            } else if (buf[i] == '\n') {
                line[len] = 0;
                marked = marked || !strcmp(trim(line), END_MARK);
                len = 0;
            } else if (len < sizeof(line) - 1) {
                line[len++] = buf[i];
            }
        }
    line[len] = 0;
    marked = marked || !strcmp(trim(line), END_MARK);
    fclose(f);
    return marked && !hole;
}

static void sync_dir_of(const char *path)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (!slash)
        return;
    *slash = 0;
    plat_flush_dir(dir);
}

/* a save cut by a power cut: the newest whole copy comes back */
static void recover(const char *path)
{
    char tmp[520], prev[520], broken[520];
    save_wait();
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    snprintf(prev, sizeof(prev), "%s.prev", path);
    bool tmp_whole = exists(tmp) && file_whole(tmp);
    if (exists(tmp) && !tmp_whole)
        remove(tmp); /* a save stopped halfway: nothing in it to keep */
    if (exists(path) && file_whole(path))
        return; /* the usual case */
    const char *from = tmp_whole ? tmp : exists(prev) ? prev : NULL;
    if (!from)
        return; /* nothing to go back to: a file of 0.14 and before loads as it is */
    if (exists(path)) {
        snprintf(broken, sizeof(broken), "%s.rotto", path);
        remove(broken);
        plat_replace(path, broken); /* kept, for the curious */
    }
    if (plat_replace(from, path) == 0) {
        sync_dir_of(path);
        log_msg(LOG_WARN, "%s was cut short (the console switched off while saving?): %s is back\n", path,
                from == tmp ? "the save being written" : "the copy before");
    }
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* The slow part of a save, flushing it to the card (tens of milliseconds on a microSD: three frames
 * of the reward's party), runs in a helper thread (0.15). One at a time: a new save, a load, a
 * profile deleted and the end of the game first wait for the one in flight (save_wait). */
static struct {
    pthread_t th;
    bool on; /* started, not yet joined */
    char tmp[520], path[512];
    double t0, ms;
    bool ok;
} J;

/* fsync the file, keep the last good one as .prev, give the new one its name, fsync the folder */
static bool finish_save(const char *tmp, const char *path)
{
    char prev[520];
    /* a write error fails the save; a file system that cannot flush (EINVAL) still gets its file */
    bool ok = plat_flush_file(tmp) == 0 || errno == EINVAL || errno == ENOTSUP;
    if (ok) {
        snprintf(prev, sizeof(prev), "%s.prev", path);
        if (exists(path) && file_whole(path)) { /* the last good one stays as .prev */
            remove(prev);
            plat_replace(path, prev);
        }
        ok = plat_replace(tmp, path) == 0;
    }
    if (ok)
        sync_dir_of(path);
    else
        remove(tmp);
    return ok;
}

static void report_save(const char *path, bool ok, double ms, const char *how)
{
    if (!ok)
        log_msg(LOG_WARN, "saving %s failed\n", path);
    else if (ms >= 10) /* a slow card */
        log_msg(LOG_INFO, "saving %s took %.0f ms (flushing it to the card%s)\n", path, ms, how);
}

static void *flush_job(void *arg)
{
    (void)arg;
    J.ok = finish_save(J.tmp, J.path);
    J.ms = now_ms() - J.t0;
    return NULL;
}

void save_wait(void)
{
    if (!J.on)
        return;
    pthread_join(J.th, NULL);
    J.on = false;
    report_save(J.path, J.ok, J.ms, ", in the background");
}

static FILE *open_tmp(const char *path, char *tmp, size_t n)
{
    save_wait(); /* (the helper may still be busy with this very file) */
    snprintf(tmp, n, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb"); /* (the same bytes on every system: a save moves between them) */
    if (!f)
        log_msg(LOG_WARN, "cannot write %s\n", tmp);
    return f;
}

static bool close_tmp(FILE *f, const char *tmp, const char *path)
{
    fputs(END_MARK "\n", f);
    bool ok = fflush(f) == 0;
    ok = (fclose(f) == 0) && ok;
    if (!ok) {
        log_msg(LOG_WARN, "saving %s failed\n", path);
        remove(tmp);
        return false;
    }
    snprintf(J.tmp, sizeof(J.tmp), "%s", tmp);
    snprintf(J.path, sizeof(J.path), "%s", path);
    J.t0 = now_ms();
    if (pthread_create(&J.th, NULL, flush_job, NULL) == 0) {
        J.on = true;
        return true; /* the helper finishes it (a failure is logged by save_wait) */
    }
    double t0 = J.t0; /* no thread: right here */
    ok = finish_save(tmp, path);
    report_save(path, ok, now_ms() - t0, "");
    return ok;
}

/* ------------------------------------------------------------------ config */

/* The numeric settings, shared by the parent's .cfg file and the in-game options. */
typedef struct {
    const char *key;
    size_t off;
    bool is_bool;
    int lo, hi;
} cfg_key_t;

static const cfg_key_t CFG_KEYS[] = {
    {"sessione_minuti", offsetof(config_t, session_minutes), false, 0, 240},
    {"livello_iniziale", offsetof(config_t, start_level), false, 1, 5},
    {"domande_per_round", offsetof(config_t, questions_per_round), false, 1, 10},
    {"volume_musica", offsetof(config_t, vol_music), false, 0, 100},
    {"volume_voce", offsetof(config_t, vol_voice), false, 0, 100},
    {"volume_effetti", offsetof(config_t, vol_sfx), false, 0, 100},
    {"vibrazione", offsetof(config_t, rumble), true, 0, 1},
    {"sblocco_giochi", offsetof(config_t, unlock_gradual), true, 0, 1},
    {"round_per_sblocco", offsetof(config_t, rounds_per_unlock), false, 1, 20},
    {"storia", offsetof(config_t, story), true, 0, 1},
    {"round_per_sfida", offsetof(config_t, rounds_per_duel), false, 1, 5},
    {"animazioni", offsetof(config_t, animations), true, 0, 1},
};

static int cfg_get(const config_t *c, const cfg_key_t *k)
{
    const char *p = (const char *)c + k->off;
    return k->is_bool ? (int)*(const bool *)p : *(const int *)p;
}

static void cfg_set(config_t *c, const cfg_key_t *k, int v)
{
    char *p = (char *)c + k->off;
    if (k->is_bool)
        *(bool *)p = v != 0;
    else
        *(int *)p = clampi(v, k->lo, k->hi);
}

void config_defaults(config_t *c)
{
    memset(c, 0, sizeof(*c));
    c->session_minutes = 15;
    c->start_level = 2;
    c->questions_per_round = 5;
    c->vol_music = 45;
    c->vol_voice = 100;
    c->vol_sfx = 70;
    c->rumble = true;
    c->unlock_gradual = true;
    c->rounds_per_unlock = 2;
    snprintf(c->child_name, sizeof(c->child_name), "DEVA");
    c->story = true;
    c->rounds_per_duel = 2;
    c->animations = true;
}

/* Keep the letters A-Z only, upper case (the big font has no accents). */
void set_name(char *dst, size_t n, const char *v)
{
    size_t k = 0;
    for (; *v && k + 1 < n && k < NAME_MAX_LEN; v++) {
        char ch = *v;
        if (ch >= 'a' && ch <= 'z')
            ch = (char)(ch - 'a' + 'A');
        if (ch >= 'A' && ch <= 'Z')
            dst[k++] = ch;
    }
    dst[k] = 0;
}

static void config_kv(void *ctx, const char *k, const char *v)
{
    config_t *c = ctx;
    for (int i = 0; i < ARRAY_LEN(CFG_KEYS); i++)
        if (!strcmp(k, CFG_KEYS[i].key)) {
            cfg_set(c, &CFG_KEYS[i], atoi(v));
            return;
        }
    if (!strcmp(k, "nome")) {
        char tmp[16];
        set_name(tmp, sizeof(tmp), v);
        if (strlen(tmp) >= 2) /* keep the default for an empty or odd value */
            memcpy(c->child_name, tmp, sizeof(tmp));
    }
}

void config_load(config_t *c, const char *data_dir)
{
    char path[512];
    config_defaults(c);
    path_join(path, sizeof(path), data_dir, "deva_adventures.cfg");
    if (!read_kv(path, config_kv, c))
        log_msg(LOG_INFO, "no %s, using defaults\n", path);
}

typedef struct {
    config_t *c;
    int *slot;
} options_ctx_t;

static void options_kv(void *ctx, const char *k, const char *v)
{
    options_ctx_t *o = ctx;
    if (!strcmp(k, "profilo"))
        *o->slot = clampi(atoi(v), 1, PROFILE_COUNT);
    else if (strcmp(k, "nome")) /* the name belongs to the profiles */
        config_kv(o->c, k, v);
}

static void options_path(char *out, size_t n, const char *save_dir)
{
    path_join(out, n, save_dir, "deva_adventures_opzioni.cfg");
}

void options_load(config_t *c, int *slot, const char *save_dir)
{
    char path[512];
    options_path(path, sizeof(path), save_dir);
    options_ctx_t o = {c, slot};
    recover(path);
    read_kv(path, options_kv, &o);
}

bool options_save(const config_t *c, const config_t *base, int slot, const char *save_dir)
{
    char path[512], tmp[520];
    options_path(path, sizeof(path), save_dir);
    FILE *f = open_tmp(path, tmp, sizeof(tmp));
    if (!f)
        return false;
    fprintf(f, "# Deva's Awesome Adventures - opzioni cambiate nel gioco (scritto dal gioco).\n"
               "# Valgono sopra deva_adventures.cfg; cancella una riga per tornare al valore del .cfg.\n");
    fprintf(f, "profilo = %d\n", clampi(slot, 1, PROFILE_COUNT));
    for (int i = 0; i < ARRAY_LEN(CFG_KEYS); i++)
        if (cfg_get(c, &CFG_KEYS[i]) != cfg_get(base, &CFG_KEYS[i]))
            fprintf(f, "%s = %d\n", CFG_KEYS[i].key, cfg_get(c, &CFG_KEYS[i]));
    return close_tmp(f, tmp, path);
}

/* ------------------------------------------------------------------ progress */

void progress_reset(progress_t *p, int start_level)
{
    memset(p, 0, sizeof(*p));
    for (int g = 0; g < GAME_COUNT; g++)
        p->level[g] = 1;
    p->level[GAME_CONTA] = clampi(start_level, 1, 5);
    for (int i = 0; i < 6; i++)
        p->worn[i] = -1;
    p->unlocked = (1u << GAME_BASE) - 1;
    p->reveal = -1;
}

static void profile_path(char *out, size_t n, const char *save_dir, int slot, const char *suffix)
{
    char name[64];
    if (slot <= 1)
        snprintf(name, sizeof(name), "deva_adventures%s", suffix);
    else
        snprintf(name, sizeof(name), "deva_adventures_%d%s", slot, suffix);
    path_join(out, n, save_dir, name);
}

void profile_log_path(char *out, size_t n, const char *save_dir, int slot)
{
    profile_path(out, n, save_dir, slot, "_log.csv");
}

void profile_diary_path(char *out, size_t n, const char *save_dir, int slot)
{
    profile_path(out, n, save_dir, slot, "_diario.csv");
}

bool profile_exists(const char *save_dir, int slot)
{
    char path[512];
    profile_path(path, sizeof(path), save_dir, slot, ".sav");
    recover(path);
    return exists(path);
}

bool profile_remove(const char *save_dir, int slot)
{
    char path[512], bak[520];
    save_wait();
    profile_path(path, sizeof(path), save_dir, slot, ".sav");
    snprintf(bak, sizeof(bak), "%s.prev", path); /* else they would come back as "cut short" */
    remove(bak);
    snprintf(bak, sizeof(bak), "%s.tmp", path);
    remove(bak);
    snprintf(bak, sizeof(bak), "%s.bak", path);
    remove(bak);
    bool ok = plat_replace(path, bak) == 0;
    for (int k = 0; k < 2; k++) { /* the logs: there may be none yet */
        (k ? profile_diary_path : profile_log_path)(path, sizeof(path), save_dir, slot);
        snprintf(bak, sizeof(bak), "%s.bak", path);
        remove(bak);
        plat_replace(path, bak);
    }
    return ok;
}

static int game_index(const char *id)
{
    for (int g = 0; g < GAME_COUNT; g++)
        if (!strcmp(GAME_IDS[g], id))
            return g;
    return -1;
}

bool game_is_unlocked(const progress_t *p, const config_t *c, int game)
{
    if (game < 0 || game >= GAME_COUNT)
        return false;
    return !c->unlock_gradual || ((p->unlocked >> game) & 1u);
}

int progress_next_locked(const progress_t *p)
{
    for (int i = 0; i < ARRAY_LEN(UNLOCK_ORDER); i++)
        if (!((p->unlocked >> UNLOCK_ORDER[i]) & 1u))
            return UNLOCK_ORDER[i];
    return -1;
}

int progress_unlock_next(progress_t *p)
{
    for (int i = 0; i < ARRAY_LEN(UNLOCK_ORDER); i++) {
        int g = UNLOCK_ORDER[i];
        if (!((p->unlocked >> g) & 1u)) {
            p->unlocked |= 1u << g;
            return g;
        }
    }
    return -1;
}

static int item_index(const char *id)
{
    for (int i = 0; i < ITEM_COUNT; i++)
        if (!strcmp(ITEMS[i].id, id))
            return i;
    return -1;
}

/* per-game counters: "<prefix>_<game> = n" */
static const struct {
    const char *prefix;
    size_t off;
    int hi;
} PER_GAME[] = {
    {"livello", offsetof(progress_t, level), 5},       {"serie", offsetof(progress_t, streak), 100},
    {"round", offsetof(progress_t, rounds), 1 << 28},  {"risposte", offsetof(progress_t, answers), 1 << 28},
    {"giuste", offsetof(progress_t, first_try), 1 << 28},
};

static void progress_kv(void *ctx, const char *k, const char *v)
{
    progress_t *p = ctx;
    const char *us = strrchr(k, '_');
    if (us) {
        int g = game_index(us + 1);
        for (int i = 0; g >= 0 && i < ARRAY_LEN(PER_GAME); i++)
            if ((size_t)(us - k) == strlen(PER_GAME[i].prefix) && !strncmp(k, PER_GAME[i].prefix, (size_t)(us - k))) {
                int *arr = (int *)((char *)p + PER_GAME[i].off);
                arr[g] = clampi(atoi(v), i == 0 ? 1 : 0, PER_GAME[i].hi);
                return;
            }
    }
    if (!strcmp(k, "serie")) /* files from version 0.1 */
        p->streak[GAME_CONTA] = clampi(atoi(v), 0, 100);
    else if (!strcmp(k, "nome"))
        set_name(p->name, sizeof(p->name), v);
    else if (!strcmp(k, "stelle_totali"))
        p->stars_total = clampi(atoi(v), 0, 1 << 28);
    else if (!strcmp(k, "round_totali"))
        p->rounds_total = clampi(atoi(v), 0, 1 << 28);
    else if (!strcmp(k, "sessioni"))
        p->sessions = clampi(atoi(v), 0, 1 << 28);
    else if (!strcmp(k, "avventura"))
        p->arc = clampi(atoi(v) - 1, 0, 3);
    else if (!strcmp(k, "capitolo"))
        p->chapter = clampi(atoi(v), 0, 5);
    else if (!strcmp(k, "carica"))
        p->charge = clampi(atoi(v), 0, 5);
    else if (!strcmp(k, "racconti"))
        p->seen = (uint32_t)strtoul(v, NULL, 10);
    else if (!strcmp(k, "epilogo"))
        p->epilogue = clampi(atoi(v), 0, 2);
    else if (!strcmp(k, "secondi_giocati"))
        p->play_seconds = strtol(v, NULL, 10) > 0 ? strtol(v, NULL, 10) : 0;
    else if (!strcmp(k, "ultima_volta"))
        snprintf(p->last_day, sizeof(p->last_day), "%.10s", v);
    else if (!strcmp(k, "giochi")) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s", v);
        for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) {
            int g = game_index(trim(t));
            if (g >= 0)
                p->unlocked |= 1u << g;
        }
    } else if (!strcmp(k, "adesivi")) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s", v);
        for (char *t = strtok(buf, ","); t; t = strtok(NULL, ","))
            for (int i = 0; i < STICKER_COUNT; i++)
                if (!strcmp(STICKERS[i], trim(t)))
                    p->stickers |= (uint64_t)1 << i;
    } else if (!strcmp(k, "da_svelare")) {
        p->reveal = game_index(v);
    } else if (!strcmp(k, "trucchi")) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s", v);
        for (char *t = strtok(buf, ","); t; t = strtok(NULL, ",")) {
            int i = item_index(trim(t));
            if (i >= 0)
                p->owned |= 1u << i;
        }
    } else {
        for (int s = 0; s < 6; s++)
            if (!strcmp(k, SLOT_KEYS[s])) {
                int i = item_index(v);
                p->worn[s] = (i >= 0 && ITEMS[i].slot == (slot_t)s) ? i : -1;
            }
    }
}

bool progress_load(progress_t *p, const char *save_dir, int slot, int start_level)
{
    char path[512];
    progress_reset(p, start_level);
    profile_path(path, sizeof(path), save_dir, slot, ".sav");
    recover(path);
    if (!read_kv(path, progress_kv, p))
        return false;
    for (int s = 0; s < 6; s++)
        if (p->worn[s] >= 0 && !(p->owned & (1u << p->worn[s])))
            p->worn[s] = -1;
    return true;
}

bool progress_save(const progress_t *p, const char *save_dir, int slot)
{
    char path[512], tmp[520];
    profile_path(path, sizeof(path), save_dir, slot, ".sav");
    FILE *f = open_tmp(path, tmp, sizeof(tmp));
    if (!f)
        return false;
    fprintf(f, "# Deva's Awesome Adventures - progressi (scritto dal gioco)\n");
    fprintf(f, "nome = %s\n", p->name);
    for (int i = 0; i < ARRAY_LEN(PER_GAME); i++) {
        const int *arr = (const int *)((const char *)p + PER_GAME[i].off);
        for (int g = 0; g < GAME_COUNT; g++)
            fprintf(f, "%s_%s = %d\n", PER_GAME[i].prefix, GAME_IDS[g], arr[g]);
    }
    fprintf(f, "trucchi = ");
    bool first = true;
    for (int i = 0; i < ITEM_COUNT; i++)
        if (p->owned & (1u << i)) {
            fprintf(f, "%s%s", first ? "" : ",", ITEMS[i].id);
            first = false;
        }
    fprintf(f, "\n");
    for (int s = 0; s < 6; s++)
        fprintf(f, "%s = %s\n", SLOT_KEYS[s], p->worn[s] >= 0 ? ITEMS[p->worn[s]].id : "");
    fprintf(f, "stelle_totali = %d\nround_totali = %d\nsessioni = %d\n", p->stars_total, p->rounds_total,
            p->sessions);
    fprintf(f, "avventura = %d\ncapitolo = %d\ncarica = %d\nracconti = %u\n", p->arc + 1, p->chapter, p->charge,
            (unsigned)p->seen);
    if (p->epilogue) /* the party after the fourth adventure: 1 due, 2 told */
        fprintf(f, "epilogo = %d\n", p->epilogue);
    fprintf(f, "giochi = ");
    first = true;
    for (int g = 0; g < GAME_COUNT; g++)
        if ((p->unlocked >> g) & 1u) {
            fprintf(f, "%s%s", first ? "" : ",", GAME_IDS[g]);
            first = false;
        }
    fprintf(f, "\n");
    if (p->reveal >= 0 && p->reveal < GAME_COUNT) /* shown in the menu next time */
        fprintf(f, "da_svelare = %s\n", GAME_IDS[p->reveal]);
    fprintf(f, "adesivi = ");
    first = true;
    for (int i = 0; i < STICKER_COUNT; i++)
        if ((p->stickers >> i) & 1u) {
            fprintf(f, "%s%s", first ? "" : ",", STICKERS[i]);
            first = false;
        }
    fprintf(f, "\n");
    fprintf(f, "secondi_giocati = %ld\nultima_volta = %s\n", p->play_seconds, p->last_day);
    return close_tmp(f, tmp, path);
}

void progress_wear(progress_t *p, int item)
{
    if (item < 0 || item >= ITEM_COUNT)
        return;
    p->owned |= 1u << item;
    p->worn[ITEMS[item].slot] = item;
}

/* ------------------------------------------------------------------ parent log */

/* the logs only grow: a row at a time, the header when the file is new */
static FILE *open_log(const char *path, const char *header)
{
    bool fresh = !exists(path);
    FILE *f = fopen(path, "ab");
    if (f && fresh)
        fprintf(f, "%s\n", header);
    return f;
}

static void stamp_now(char *out, size_t n)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (!tm || !strftime(out, n, "%Y-%m-%d %H:%M:%S", tm))
        snprintf(out, n, "%ld", (long)now);
}

void log_answer(const char *path, int session, const char *game, int level, const char *subject,
                const char *target, const char *chosen, bool correct, int attempt, double secs, int replays,
                long at)
{
    FILE *f = open_log(path, "data;sessione;gioco;livello;domanda;risposta_giusta;scelta;esito;tentativo;"
                             "secondi_risposta;riascolti;secondo_sessione");
    if (!f)
        return;
    char stamp[32], took[16] = "";
    stamp_now(stamp, sizeof(stamp));
    if (secs >= 0)
        snprintf(took, sizeof(took), "%.1f", secs);
    fprintf(f, "%s;%d;%s;%d;%s;%s;%s;%s;%d;%s;%d;%ld\n", stamp, session, game, level, subject, target, chosen,
            correct ? "giusta" : "sbagliata", attempt, took, replays, at);
    fclose(f);
}

void log_diary(const char *path, int session, long at, const char *event, const char *detail)
{
    FILE *f = open_log(path, "data;sessione;secondo_sessione;evento;dettaglio");
    if (!f)
        return;
    char stamp[32];
    stamp_now(stamp, sizeof(stamp));
    fprintf(f, "%s;%d;%ld;%s;%s\n", stamp, session, at, event, detail);
    fclose(f);
}
