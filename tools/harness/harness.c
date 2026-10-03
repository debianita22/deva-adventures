/* Headless libretro frontend for automated Deva's Awesome Adventures playthroughs.
 *
 *   harness <core.so> <system_dir> <save_dir> <out_dir> [options]
 *     --plan STR      answers per question, cycled: c = right first time,
 *                     w = one mistake, h = two mistakes (guided help)
 *     --games STR     game picked at each menu visit, cycled: c conta, p parole,
 *                     s sequenze, b balla, n nome, m memory, r ritmo, d dove,
 *                     e emozioni, o storie, g ginnastica, t trucco, f forme,
 *                     l lettere, h ombre, w sentiero, q negozio, u misure
 *                     (default cpsb)
 *     --frames N      stop after N frames (default 60000)
 *     --shots         save PNG screenshots at key moments in out_dir
 *     --press-at F:K  press key K (L R U D A B X Y S=start E=select) at frame F,
 *                     over the bot (repeatable): tests of a single button
 *     --shot-at F:NAME a PNG screenshot NAME.png at frame F (repeatable; the
 *                     frames of a game are in log.txt), e.g. for release sheets
 *     --video FILE    record an MP4 (needs ffmpeg), 2x nearest-neighbour
 *     --tour          visit the main menu first (dressing room, album, saves with
 *                     a new profile typed on the keyboard, options held open),
 *                     pause a game once, and at the end leave through "Esci"
 *     --rivedi        first open the album on "LA STORIA 4" and tell that
 *                     adventure again (red button), then play as usual
 *     --idle-acts     never press during the kind acts of the endings (dance,
 *                     lantern, lullaby, ball, fireworks): they must end by
 *                     themselves (0.12)
 *     --audio FILE    raw s16le stereo 44.1 kHz of the whole run
 *     --monkey SEED   no bot: a small child with the console (0.14): random
 *                     behaviours, a few seconds each - trying an answer, mashing
 *                     the red button, holding it down, the cross at random, any
 *                     button, START, everything at once, putting it down - to find
 *                     stuck scenes, answers given by chance, accidental exits
 *
 * The bot reads the core's "BOT ..." debug log lines to know the scene, the
 * right answer and the current highlight, then presses buttons like a child
 * would (with pauses). Prints a summary and exits non-zero on problems.
 * SPDX-License-Identifier: MIT
 */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libretro.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define W 320
#define H 240

static struct {
    void (*init)(void);
    void (*deinit)(void);
    void (*set_environment)(retro_environment_t);
    void (*set_video_refresh)(retro_video_refresh_t);
    void (*set_audio_sample)(retro_audio_sample_t);
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*set_input_poll)(retro_input_poll_t);
    void (*set_input_state)(retro_input_state_t);
    void (*get_system_av_info)(struct retro_system_av_info *);
    bool (*load_game)(const struct retro_game_info *);
    void (*unload_game)(void);
    void (*run)(void);
} core;

static const char *sys_dir, *save_dir, *out_dir;
static uint16_t frame_buf[W * H];
static long frame_no, dupes, fresh, rumbles, warnings, errors;
static bool shutdown_req, want_shots;
static FILE *video_pipe, *audio_raw, *log_file;
static long audio_frames;

/* ------------------------------------------------------------------ bot */
enum { G_CONTA, G_PAROLE, G_SEQUENZE, G_BALLA, G_NOME, G_MEMORY, G_RITMO, G_DOVE, G_EMOZIONI, G_STORIE,
       G_GINNASTICA, G_TRUCCO, G_FORME, G_LETTERE, G_OMBRE, G_SENTIERO, G_NEGOZIO, G_MISURE, G_N };
static const char *GAME_NAMES[G_N] = {"conta",    "parole", "sequenze", "balla",      "nome",
                                      "memory",   "ritmo",  "dove",     "emozioni",   "storie",
                                      "ginnastica", "trucco", "forme", "lettere", "ombre", "sentiero",
                                      "negozio", "misure"};
static const char GAME_KEYS[G_N + 1] = "cpsbnmrdeogtflhwqu"; /* --games letters (q: i quattrini) */
static const char SHOT_PREFIX[G_N + 1] = "1234567abcdefghijk"; /* screenshot names, per game */

/* memory: the layout, what is found, the cursor */
static int mem_cols, mem_n, mem_item[16], mem_cur, mem_first = -1, mem_misses_pair;
static bool mem_found[16], mem_ready;
static long mem_wait;
/* ritmo: when to clap */
static int rit_at[16], rit_n, rit_k, rit_turns;
static long rit_start = -1;
/* dove: first move towards the right place and towards a wrong one ('-' = there) */
static char dove_goal = '?', dove_other = '?';
static bool dove_ready;
/* the tale: map, menu duel card, duels */
static bool map_ready, menu_sfida;
static int duels, duels_won, map_visits, story_done, racconti;
/* the kind acts of the endings (0.12): what to do, and when */
static char act_kind[16];         /* "dance", "lantern", "lullaby", "ball", "fireworks"; "" = none */
static long act_next = -1;        /* frame of the next press */
static int act_presses, acts_done, acts_auto, act_waits, recaps, epilogue_done;
static bool act_ball_mine, act_wait_tested, idle_acts;

static char scene[16] = "?";
static long tour_at = -1;           /* frame of the last scene change (the tour waits after it) */
static int tas_sel;                 /* the letter keyboard: highlighted key */
static int pauses, profiles_new, profiles_deleted;
static const char *plan = "c";   /* per answer: c right, w one mistake, h two mistakes (guided help) */
static const char *games = "cpsb"; /* per menu visit: c conta, p parole, s sequenze, b balla */
static int plan_i, question_no, correct_idx = -1, sel = 1, attempts, answers, wrongs, hints, rewards;
static unsigned passi_key[3]; /* one-step "passi" duel question: the arrow of each card, 0 = other questions */
static bool off[3], ready, waiting_reward_continue;
static long next_action, press_until, reward_at, offer_at, title_at = -1, menu_at = -1;
static uint32_t press_mask;
static int level_ups, level_downs;
static int menu_sel, menu_visits, unlocks;
static bool menu_sel_locked;
/* 0.14: the menu shows the open games and one "?" (the next one): the bot looks for its game by name,
   right to the end, then left; not there (or locked) = the next game of the plan */
static char menu_game[16];
static int menu_scan_dir = 1, menu_turns, menu_last_sel = -1;
static char prova_step;
static long prova_at = -1;
static int game_q[G_N], game_maxlvl[G_N], game_rounds[G_N], game_shots[G_N], game_hint_shots[G_N],
    game_ok_shots[G_N];
static int cur_game = -1;
/* balla con me */
static int balla_seq[8], balla_len, balla_i, balla_turn_shots, balla_show_shots;
static bool balla_ready, balla_guided;
/* ginnastica: the routine to do, as buttons */
static unsigned gi_seq[16];
static int gi_len, gi_i, gi_turn_shots, gi_listen_shots;
static bool gi_ready, gi_guided;
/* il sentiero: the shortest program, the program the core holds, the one the bot wants to run */
static char sen_sol[16], sen_cur[16], sen_want[16];
static bool sen_ready, sen_guided;
static int sen_run_shots;
static long sen_wait_until = -1; /* a key pressed: waiting for the core to echo it (else press again) */

typedef struct {
    char name[64];
    long at;
} shot_t;
static shot_t shots[256];
static int nshots;

#define MAX_TIMES 400000
static float times[MAX_TIMES];

static int cmp_float(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : x > y;
}

static void save_png(const char *name);

static void schedule_shot(const char *name, long delay)
{
    if (!want_shots || nshots >= 256)
        return;
    snprintf(shots[nshots].name, sizeof(shots[0].name), "%s", name);
    shots[nshots].at = frame_no + delay;
    nshots++;
}

static void press(unsigned id)
{
    if (getenv("DEVA_TOUR_DEBUG"))
        fprintf(stderr, "%06ld press %u scene=%s\n", frame_no, id, scene);
    press_mask = 1u << id;
    press_until = frame_no + 3;
    next_action = frame_no + 14; /* a child presses, not a machine gun */
}

static int game_index(const char *name)
{
    for (int i = 0; i < G_N; i++)
        if (!strncmp(name, GAME_NAMES[i], strlen(GAME_NAMES[i])))
            return i;
    return -1;
}

static char plan_now(void) { return plan[plan_i % strlen(plan)]; }

static void on_bot(const char *m)
{
    char nm[96];
    if (!strncmp(m, "scene=", 6)) {
        snprintf(scene, sizeof(scene), "%.15s", m + 6);
        scene[strcspn(scene, "\n")] = 0;
        ready = balla_ready = false;
        int g = game_index(scene);
        cur_game = g;
        map_ready = false;
        tour_at = frame_no;
        if (!strcmp(scene, "title")) {
            title_at = frame_no;
            schedule_shot("01_titolo", 100);
        } else if (!strcmp(scene, "menu")) {
            menu_at = frame_no;
            menu_scan_dir = 1;
            menu_turns = 0;
            menu_last_sel = -1;
            if (menu_visits == 0)
                schedule_shot("02_zmenu", 120);
        } else if (!strcmp(scene, "fine")) {
            schedule_shot("90_fine_saluto", 60);
            schedule_shot("91_fine_nanna", 520);
            schedule_shot("92_fine_sipario", 690);
        }
    } else if (!strncmp(m, "tastiera sel=", 13)) {
        tas_sel = atoi(m + 13);
    } else if (!strncmp(m, "pause open", 10)) {
        pauses++;
    } else if (!strncmp(m, "profile new", 11)) {
        profiles_new++;
    } else if (!strncmp(m, "profile delete", 14)) {
        profiles_deleted++;
    } else if (!strncmp(m, "prova step=", 11)) {
        prova_step = m[11];
        prova_at = frame_no;
        if (prova_step == 'c')
            schedule_shot("02_prova_croce", 100);
        else if (prova_step == 'r')
            schedule_shot("02_prova_rosso", 150);
    } else if (!strncmp(m, "menu sfida=", 11)) {
        static int sfida_menu_shots;
        menu_sfida = m[11] == '1';
        if (menu_sfida && sfida_menu_shots++ < 1)
            schedule_shot("n0_menu_sfida", 150);
    } else if (!strncmp(m, "mappa chapter=", 14)) {
        int ch = atoi(m + 14);
        map_visits++;
        snprintf(nm, sizeof(nm), "m%d_mappa_%d", ch, map_visits);
        if (map_visits <= 8)
            schedule_shot(nm, strstr(m, "event=1") ? 60 : 160);
    } else if (!strncmp(m, "mappa ready", 11)) {
        map_ready = true;
        next_action = frame_no + 40;
    } else if (!strncmp(m, "racconto act=", 13)) {
        /* "racconto act=dance ready", "...act=ball turn=deva", "...act=lullaby done" */
        char kind[16] = "", what[24] = "";
        sscanf(m + 13, "%15s %23s", kind, what);
        if (!strcmp(what, "ready")) {
            snprintf(act_kind, sizeof(act_kind), "%s", kind);
            act_next = idle_acts ? -1 : frame_no + 30; /* a child takes a moment */
        } else if (!strcmp(what, "turn=deva")) {
            act_ball_mine = true;
            act_next = idle_acts ? -1 : frame_no + 25;
        } else if (!strcmp(what, "turn=king")) {
            act_ball_mine = false;
        } else if (!strcmp(what, "aspetta")) {
            act_waits++;
        } else if (!strcmp(what, "auto")) {
            acts_auto++;
        } else if (!strcmp(what, "done")) {
            acts_done++;
            act_kind[0] = 0;
            act_next = -1;
        }
    } else if (!strncmp(m, "mappa recap", 11)) {
        recaps++;
        schedule_shot("m9_mappa_bentornata", 260);
    } else if (!strncmp(m, "racconto_end epilogo", 20)) {
        epilogue_done = 1;
    } else if (!strncmp(m, "racconto ", 9)) {
        char name[16] = "";
        int step = 0;
        sscanf(m, "racconto %15s step=%d", name, &step);
        racconti++;
        static const struct {
            const char *name;
            int step, delay;
        } WANT[] = {{"prologo", 1, 60}, {"prologo", 2, 50}, {"prologo", 3, 100}, {"prologo", 4, 125},
                    {"prologo", 5, 200}, {"prologo", 6, 60}, {"prologo", 7, 50}, {"sfida", 1, 40},
                    {"vittoria", 1, 30}, {"vittoria", 2, 50}, {"finale", 1, 40}, {"finale", 3, 70},
                    {"finale", 4, 200}, {"finale", 5, 30}, {"finale", 6, 60}, {"finale", 7, 60},
                    /* the second adventure */
                    {"prologo2", 1, 60}, {"prologo2", 2, 70}, {"prologo2", 3, 80}, {"prologo2", 4, 235},
                    {"prologo2", 5, 210}, {"prologo2", 6, 90}, {"prologo2", 7, 50}, {"finale2", 1, 40},
                    {"finale2", 3, 150}, {"finale2", 4, 60}, {"finale2", 5, 80}, {"finale2", 6, 120},
                    {"finale2", 7, 60},
                    /* the third adventure */
                    {"prologo3", 1, 80}, {"prologo3", 2, 90}, {"prologo3", 3, 60}, {"prologo3", 4, 120},
                    {"prologo3", 5, 200}, {"prologo3", 6, 60}, {"prologo3", 7, 50}, {"finale3", 1, 60},
                    {"finale3", 2, 90}, {"finale3", 3, 80}, {"finale3", 4, 210}, {"finale3", 5, 90},
                    {"finale3", 6, 30}, {"finale3", 7, 120}, {"finale3", 8, 60}, {"finale3", 9, 90},
                    /* the fourth adventure */
                    {"prologo4", 1, 90}, {"prologo4", 2, 90}, {"prologo4", 3, 70}, {"prologo4", 4, 120},
                    {"prologo4", 5, 200}, {"prologo4", 6, 60}, {"prologo4", 7, 50}, {"finale4", 1, 60},
                    {"finale4", 2, 60}, {"finale4", 3, 70}, {"finale4", 4, 240}, {"finale4", 5, 30},
                    {"finale4", 6, 120}, {"finale4", 7, 60}, {"finale4", 8, 90},
                    /* the party for Deva */
                    {"epilogo", 1, 60}, {"epilogo", 2, 70}, {"epilogo", 3, 70}, {"epilogo", 4, 70},
                    {"epilogo", 5, 70}, {"epilogo", 6, 100}, {"epilogo", 7, 160}, {"epilogo", 8, 80}};
        static int sfida_shots, vitt_shots;
        for (int i = 0; i < (int)(sizeof(WANT) / sizeof(WANT[0])); i++)
            if (!strcmp(WANT[i].name, name) && WANT[i].step == step) {
                if (!strcmp(name, "sfida") && sfida_shots++ >= 5)
                    break;
                if (!strcmp(name, "vittoria") && step == 1 && vitt_shots++ >= 2)
                    break;
                snprintf(nm, sizeof(nm), "r_%s_%d_%d", name, step, racconti);
                schedule_shot(nm, WANT[i].delay);
            }
    } else if (!strncmp(m, "racconto_end finale", 19)) {
        story_done = 1;
    } else if (!strncmp(m, "duel foe=", 9)) {
        duels++;
        char foe[16] = "";
        sscanf(m, "duel foe=%15s", foe);
        snprintf(nm, sizeof(nm), "s%d_sfida_%s", duels, foe);
        schedule_shot(nm, 200);
    } else if (!strncmp(m, "duel orb=1/", 11)) {
        static int orb_shots;
        if (orb_shots++ < 2) {
            snprintf(nm, sizeof(nm), "s%d_sfida_incantesimo", duels);
            schedule_shot(nm, 0);
        }
    } else if (!strncmp(m, "duel question=", 14) && strstr(m, "game=strega")) {
        schedule_shot("s9_strega_piange", 170);
    } else if (!strncmp(m, "duel question=", 14) && strstr(m, "game=stregone")) {
        schedule_shot("s9_stregone_paura", 170);
    } else if (!strncmp(m, "duel question=", 14) && strstr(m, "game=orco")) {
        schedule_shot("s9_orco_sbuffa", 170);
    } else if (!strncmp(m, "duel question=", 14) && strstr(m, "game=re\n")) {
        schedule_shot("s9_re_piange", 170);
    } else if (!strncmp(m, "duel question=", 14)) { /* the duel questions of the non-card games */
        static const char *const KINDS[] = {"balla", "ritmo", "dove", "memory", "nome", "lettere", "ombre",
                                            "sentiero", "negozio", "misure"};
        static int kind_shots[10];
        const char *g = strstr(m, "game=");
        for (int k = 0; g && k < 10; k++) {
            size_t len = strlen(KINDS[k]);
            if (!strncmp(g + 5, KINDS[k], len) && (g[5 + len] == 0 || g[5 + len] == '\n') && kind_shots[k]++ < 1) {
                snprintf(nm, sizeof(nm), "q_duello_%s", KINDS[k]);
                schedule_shot(nm, k == 1 ? 150 : 110);
            }
        }
    } else if (!strncmp(m, "duel won=", 9)) {
        duels_won++;
    } else if (!strncmp(m, "menu sel=", 9)) {
        menu_sel = atoi(m + 9);
        menu_sel_locked = strstr(m, "locked=1") != NULL;
        const char *gp = strstr(m, "game=");
        if (gp)
            sscanf(gp + 5, "%15s", menu_game);
    } else if (!strncmp(m, "unlock game=", 12)) {
        unlocks++;
    } else if (!strncmp(m, "menu_unlocked", 13)) {
        menu_sel_locked = false;
        if (unlocks <= 2)
            schedule_shot(unlocks == 1 ? "85_menu_sblocco" : "86_menu_sblocco", 10);
    } else if (!strncmp(m, "menu_pick", 9)) {
        const char *gp = strstr(m, "game=");
        int g = gp ? game_index(gp + 5) : -1;
        if (g >= 0) { /* a duel does not use up the game planned for this visit */
            menu_visits++;
            game_rounds[g]++;
        }
    } else if (!strncmp(m, "question", 8)) {
        char game[16] = "", subject[64] = "", opts[96] = "";
        int lvl = 0;
        sscanf(m, "question game=%15s level=%d subject=%63s opts=%95s correct=%d", game, &lvl, subject, opts,
               &correct_idx);
        int step = 1;
        const char *sp = strstr(m, " step=");
        if (sp)
            step = atoi(sp + 6);
        int g = game_index(game);
        if (g >= 0) {
            game_q[g]++;
            if (lvl > game_maxlvl[g])
                game_maxlvl[g] = lvl;
        }
        if (step <= 1)
            question_no++;
        attempts = 0;
        off[0] = off[1] = off[2] = false;
        const char *op = strstr(m, " off=");
        if (op && strlen(op) >= 8) /* cards already used by earlier steps */
            for (int i = 0; i < 3; i++)
                off[i] = op[5 + i] == '1';
        sel = 1;
        ready = false;
        /* the one-step dance question: the arrow of the cross picks the card with that arrow */
        passi_key[0] = passi_key[1] = passi_key[2] = 0;
        if (!strncmp(subject, "passi:", 6) && !strchr(subject, '-')) {
            char buf[96];
            snprintf(buf, sizeof(buf), "%s", opts);
            int i = 0;
            for (char *t = strtok(buf, ","); t && i < 3; t = strtok(NULL, ","), i++)
                passi_key[i] = !strcmp(t, "su")    ? RETRO_DEVICE_ID_JOYPAD_UP
                               : !strcmp(t, "giu") ? RETRO_DEVICE_ID_JOYPAD_DOWN
                               : !strcmp(t, "sx")  ? RETRO_DEVICE_ID_JOYPAD_LEFT
                                                   : RETRO_DEVICE_ID_JOYPAD_RIGHT;
        }
    } else if (!strncmp(m, "ready", 5)) {
        sscanf(m, "ready sel=%d", &sel);
        if (!ready && cur_game >= 0 && attempts == 0 && game_shots[cur_game] < 3) {
            snprintf(nm, sizeof(nm), "%c%d_%s_domanda%d", SHOT_PREFIX[cur_game], game_shots[cur_game],
                     GAME_NAMES[cur_game], game_shots[cur_game] + 1);
            game_shots[cur_game]++;
            schedule_shot(nm, 50);
        }
        if (!ready && attempts == 0 && question_no == 1) {
            /* first question of the session: the spoken tutorial with the console */
            schedule_shot("03_tutorial_croce", 150);
            schedule_shot("03_tutorial_croce_b", 160); /* the highlight blinks: one of the two is lit */
            schedule_shot("04_tutorial_rosso", 240);
            schedule_shot("04_tutorial_rosso_b", 250);
            next_action = frame_no + 300;
        } else if (!ready && attempts == 0 && question_no == 2) {
            schedule_shot("05_tutorial_giallo", 150);
            schedule_shot("05_tutorial_giallo_b", 160);
            next_action = frame_no + 220;
        }
        ready = true;
        if (next_action < frame_no + 20)
            next_action = frame_no + 20;
        if (cur_game >= 0 && game_shots[cur_game] <= 3 && next_action < frame_no + 70)
            next_action = frame_no + 70; /* let the screenshot happen first */
    } else if (!strncmp(m, "sel=", 4)) {
        sel = atoi(m + 4);
    } else if (!strncmp(m, "answer", 6)) {
        int pick = 0, ok = 0, att = 0;
        sscanf(m, "answer pick=%d ok=%d attempt=%d", &pick, &ok, &att);
        answers++;
        ready = balla_ready = false;
        if (ok) {
            plan_i++;
            if (cur_game >= 0 && game_ok_shots[cur_game]++ == 0) {
                snprintf(nm, sizeof(nm), "%c8_%s_giusta", SHOT_PREFIX[cur_game], GAME_NAMES[cur_game]);
                schedule_shot(nm, 30);
            }
        } else {
            wrongs++;
            if (pick >= 0 && pick < 3 && strcmp(scene, "balla"))
                off[pick] = true;
            attempts++;
        }
    } else if (!strncmp(m, "hint_counting", 13)) {
        /* conta: the guided count is on screen */
    } else if (!strncmp(m, "hint", 4)) {
        hints++;
        if (cur_game >= 0 && game_hint_shots[cur_game]++ == 0) {
            snprintf(nm, sizeof(nm), "%c7_%s_aiuto", SHOT_PREFIX[cur_game], GAME_NAMES[cur_game]);
            schedule_shot(nm, cur_game == G_BALLA ? 200 : 110);
        }
    } else if (!strncmp(m, "level_up", 8)) {
        level_ups++;
    } else if (!strncmp(m, "level_down", 10)) {
        level_downs++;
    } else if (!strncmp(m, "offer", 5)) {
        offer_at = frame_no;
        if (rewards == 0)
            schedule_shot("80_premio_scelta", 200);
    } else if (!strncmp(m, "reward", 6)) {
        rewards++;
        reward_at = frame_no;
        waiting_reward_continue = true;
        if (rewards == 1)
            schedule_shot("81_premio_indossato", 90);
    } else if (!strncmp(m, "memory grid=", 12)) {
        int cols = 0, rows = 0, lvl = 0;
        char lay[128] = "";
        sscanf(m, "memory grid=%dx%d level=%d layout=%127s", &cols, &rows, &lvl, lay);
        mem_cols = cols;
        mem_n = 0;
        for (char *t = strtok(lay, ","); t && mem_n < 16; t = strtok(NULL, ","))
            mem_item[mem_n++] = atoi(t);
        memset(mem_found, 0, sizeof(mem_found));
        mem_cur = 0;
        mem_first = -1;
        mem_ready = false;
        mem_misses_pair = 0;
        game_q[G_MEMORY]++;
        if (lvl > game_maxlvl[G_MEMORY])
            game_maxlvl[G_MEMORY] = lvl;
        if (game_shots[G_MEMORY]++ < 2) {
            snprintf(nm, sizeof(nm), "6%d_memory_griglia", game_shots[G_MEMORY]);
            schedule_shot(nm, 150);
        }
    } else if (!strncmp(m, "memory_ready", 12)) {
        mem_ready = true;
        mem_wait = frame_no + 20;
    } else if (!strncmp(m, "memory cursor=", 14)) {
        mem_cur = atoi(m + 14);
    } else if (!strncmp(m, "memory_match", 12)) {
        mem_wait = frame_no + 40;
        plan_i++;
        mem_misses_pair = 0;
        if (game_ok_shots[G_MEMORY]++ == 0)
            schedule_shot("68_memory_coppia", 8);
    } else if (!strncmp(m, "memory_miss", 11)) {
        wrongs++;
        mem_misses_pair++;
        mem_wait = frame_no + 90;
        if (mem_misses_pair == 1 && game_hint_shots[G_MEMORY] == 0)
            schedule_shot("66_memory_sbagliata", 20);
    } else if (!strncmp(m, "memory_flip", 11)) {
        /* the bot's own flips are tracked in bot_step */
    } else if (!strncmp(m, "memory_done", 11)) {
        mem_ready = false;
        answers++;
    } else if (!strncmp(m, "ritmo pattern=", 14)) {
        char pat[96] = "";
        int lvl = 0;
        sscanf(m, "ritmo pattern=%95s level=%d", pat, &lvl);
        rit_n = 0;
        for (char *t = strtok(pat, ","); t && rit_n < 16; t = strtok(NULL, ","))
            rit_at[rit_n++] = atoi(t);
        rit_turns = 0;
        attempts = 0;
        game_q[G_RITMO]++;
        question_no++;
        if (lvl > game_maxlvl[G_RITMO])
            game_maxlvl[G_RITMO] = lvl;
    } else if (!strncmp(m, "ritmo_show", 10)) {
        rit_start = -1;
        if (game_shots[G_RITMO] < 1) {
            game_shots[G_RITMO]++;
            schedule_shot("70_ritmo_ascolta", 110);
        }
    } else if (!strncmp(m, "ritmo_turn", 10)) {
        rit_turns++;
        rit_k = 0;
        rit_start = frame_no + 50;
        if (rit_turns == 1 && game_shots[G_RITMO] < 2) {
            game_shots[G_RITMO]++;
            schedule_shot("71_ritmo_tocca", 50 + rit_at[rit_n - 1] + 30);
        }
    } else if (!strncmp(m, "dove q=", 7)) {
        int lvl = 0;
        const char *lp = strstr(m, "level=");
        if (lp)
            lvl = atoi(lp + 6);
        game_q[G_DOVE]++;
        if (lvl > game_maxlvl[G_DOVE])
            game_maxlvl[G_DOVE] = lvl;
        question_no++;
        attempts = 0;
        dove_ready = false;
    } else if (!strncmp(m, "dove_ready", 10) || !strncmp(m, "dove at=", 8)) {
        const char *gp = strstr(m, "goal="), *op = strstr(m, "other=");
        dove_goal = gp ? gp[5] : '?';
        dove_other = op ? op[6] : '?';
        if (!strncmp(m, "dove_ready", 10)) {
            dove_ready = true;
            next_action = frame_no + 30;
            if (attempts == 0 && game_shots[G_DOVE] < 3) {
                snprintf(nm, sizeof(nm), "a%d_dove_domanda%d", game_shots[G_DOVE], game_shots[G_DOVE] + 1);
                game_shots[G_DOVE]++;
                schedule_shot(nm, 60);
                next_action = frame_no + 80;
            }
        }
    } else if (!strncmp(m, "ginnastica seq=", 15)) {
        char seq[64] = "";
        int lvl = 0, hid = 0;
        sscanf(m, "ginnastica seq=%63s level=%d hidden=%d", seq, &lvl, &hid);
        gi_len = 0;
        for (char *tok = strtok(seq, ","); tok && gi_len < 16; tok = strtok(NULL, ",")) {
            gi_seq[gi_len] = tok[0] == 'S'   ? RETRO_DEVICE_ID_JOYPAD_A
                             : tok[0] == 'C' ? RETRO_DEVICE_ID_JOYPAD_DOWN
                             : tok[0] == 'R' ? (gi_len % 2 ? RETRO_DEVICE_ID_JOYPAD_LEFT : RETRO_DEVICE_ID_JOYPAD_RIGHT)
                                             : RETRO_DEVICE_ID_JOYPAD_UP;
            gi_len++; /* after: reading and moving it in one statement is undefined */
        }
        game_q[G_GINNASTICA]++;
        if (lvl > game_maxlvl[G_GINNASTICA])
            game_maxlvl[G_GINNASTICA] = lvl;
        question_no++;
        attempts = 0;
    } else if (!strncmp(m, "ginnastica_do move=", 19)) {
        static bool shot_c, shot_r, shot_s;
        char mv = m[19];
        if (mv == 'C' && !shot_c) {
            shot_c = true;
            schedule_shot("d6_ginnastica_capriola", 20);
            schedule_shot("d6_ginnastica_capriola_b", 12);
            schedule_shot("d6_ginnastica_capriola_c", 28);
        } else if (mv == 'R' && !shot_r) {
            shot_r = true;
            schedule_shot("d7_ginnastica_ruota", 18);
            schedule_shot("d7_ginnastica_ruota_b", 9);
            schedule_shot("d7_ginnastica_ruota_c", 28);
        } else if (mv == 'S' && !shot_s) {
            shot_s = true;
            schedule_shot("d5_ginnastica_salto", 14);
        }
    } else if (!strncmp(m, "ginnastica_listen", 17)) {
        gi_ready = false;
        if (gi_listen_shots < 2) {
            snprintf(nm, sizeof(nm), "d%d_ginnastica_ascolta", gi_listen_shots);
            schedule_shot(nm, 70);
            gi_listen_shots++;
        }
    } else if (!strncmp(m, "ginnastica_turn", 15)) {
        gi_guided = strstr(m, "guided=1") != NULL;
        gi_ready = true;
        gi_i = 0;
        next_action = frame_no + 40;
        if (gi_turn_shots < 3) {
            snprintf(nm, sizeof(nm), "d%d_ginnastica_tocca", 3 + gi_turn_shots);
            schedule_shot(nm, 100);
            gi_turn_shots++;
        }
    } else if (!strncmp(m, "sentiero garden", 15)) {
        int lvl = 0;
        const char *lp = strstr(m, "level=");
        if (lp)
            lvl = atoi(lp + 6);
        game_q[G_SENTIERO]++;
        if (lvl > game_maxlvl[G_SENTIERO])
            game_maxlvl[G_SENTIERO] = lvl;
        question_no++;
        attempts = 0;
        sen_cur[0] = 0;
        sen_ready = false;
        if (game_shots[G_SENTIERO] < 3) {
            snprintf(nm, sizeof(nm), "i%d_sentiero_domanda%d", game_shots[G_SENTIERO], game_shots[G_SENTIERO] + 1);
            game_shots[G_SENTIERO]++;
            schedule_shot(nm, 90);
        }
    } else if (!strncmp(m, "sentiero ready", 14)) {
        /* what to run now: the shortest way, or (the plan wants a mistake) one that does not arrive */
        int prog = 0, guided = 0;
        const char *sp = strstr(m, "sol="), *pp = strstr(m, "prog="), *gp = strstr(m, "guided=");
        if (sp)
            sscanf(sp + 4, "%15s", sen_sol);
        if (pp)
            prog = atoi(pp + 5);
        if (gp)
            guided = atoi(gp + 7);
        if (prog < (int)strlen(sen_cur))
            sen_cur[prog] = 0;
        sen_guided = guided != 0;
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        snprintf(sen_want, sizeof(sen_want), "%s", sen_sol);
        size_t n = strlen(sen_want);
        if (!sen_guided && attempts < need_wrong && n > 0) {
            if (attempts == 0 && n > 1) {
                sen_want[n - 1] = 0; /* one arrow short: "quasi!" */
            } else {              /* the first arrow the other way: a bump or the edge */
                char c = sen_want[0];
                sen_want[0] = c == 'U' ? 'D' : c == 'D' ? 'U' : c == 'L' ? 'R' : 'L';
            }
        }
        sen_ready = true;
        next_action = frame_no + (game_shots[G_SENTIERO] <= 3 ? 110 : 50);
    } else if (!strncmp(m, "sentiero add=", 13)) {
        size_t n = strlen(sen_cur);
        if (n + 1 < sizeof(sen_cur)) {
            sen_cur[n] = m[13];
            sen_cur[n + 1] = 0;
        }
        if (sen_wait_until >= 0)
            sen_ready = true, sen_wait_until = -1;
    } else if (!strncmp(m, "sentiero del", 12)) {
        size_t n = strlen(sen_cur);
        if (n)
            sen_cur[n - 1] = 0;
        if (sen_wait_until >= 0)
            sen_ready = true, sen_wait_until = -1;
    } else if (!strncmp(m, "sentiero run", 12)) {
        sen_ready = false;
        sen_wait_until = -1;
        if (sen_run_shots < 2) {
            snprintf(nm, sizeof(nm), "i%d_sentiero_cammina", 5 + sen_run_shots);
            schedule_shot(nm, 30);
            sen_run_shots++;
        }
    } else if (!strncmp(m, "balla seq=", 10)) {
        char seq[64] = "";
        int lvl = 0, hid = 0;
        sscanf(m, "balla seq=%63s level=%d hidden=%d", seq, &lvl, &hid);
        balla_len = 0;
        for (char *tok = strtok(seq, "-"); tok && balla_len < 8; tok = strtok(NULL, "-"))
            balla_seq[balla_len++] = !strcmp(tok, "su")    ? RETRO_DEVICE_ID_JOYPAD_UP
                                     : !strcmp(tok, "giu") ? RETRO_DEVICE_ID_JOYPAD_DOWN
                                     : !strcmp(tok, "sx")  ? RETRO_DEVICE_ID_JOYPAD_LEFT
                                                           : RETRO_DEVICE_ID_JOYPAD_RIGHT;
        game_q[G_BALLA]++;
        if (lvl > game_maxlvl[G_BALLA])
            game_maxlvl[G_BALLA] = lvl;
        question_no++;
        attempts = 0;
    } else if (!strncmp(m, "balla_show", 10)) {
        balla_ready = false;
        if (balla_show_shots < 2) {
            snprintf(nm, sizeof(nm), "4%d_balla_guarda", balla_show_shots);
            schedule_shot(nm, 80);
            balla_show_shots++;
        }
    } else if (!strncmp(m, "balla_turn", 10)) {
        balla_guided = strstr(m, "guided=1") != NULL;
        balla_ready = true;
        balla_i = 0;
        next_action = frame_no + 40;
        if (balla_turn_shots < 2) {
            snprintf(nm, sizeof(nm), "4%d_balla_tocca", 3 + balla_turn_shots);
            schedule_shot(nm, 30);
            balla_turn_shots++;
        }
    }
}


/* ------------------------------------------------------------------ scripted tour of the menus */
typedef struct {
    const char *scene; /* run when this scene is on */
    const char *keys;  /* L R U D A B X Y S(tart, held for the pause) H(old A) G(rown-ups: L+R held)
                          Z (X+Y held); "type:NAME!" on the keyboard */
    int wait;          /* frames after the keys */
    const char *shot;  /* screenshot at the end of the wait */
} tstep_t;

static const tstep_t TOUR_MAIN[] = {
    {"title", "", 150, "t0_menu_principale"},
    {"title", "R", 70, "t1_menu_camerino"},
    {"title", "A", 10, NULL},
    {"camerino", "", 150, "c0_camerino"},
    {"camerino", "RA", 110, "c1_camerino_indossa"},
    {"camerino", "DA", 60, "c2_camerino_bloccato"},
    {"camerino", "B", 10, NULL},
    {"title", "RRA", 10, NULL},                       /* (0.14: back from the dressing room on "Gioca") */
    {"album", "", 120, "a0_album_storia"},
    {"album", "R", 60, "a1_album_giochi"},
    {"album", "R", 60, "a2_album_trucchi"},
    {"album", "R", 60, "a3_album_stelle"},
    {"album", "R", 60, "a4_album_5"},   /* more pages with the stickers (else: stays on the last) */
    {"album", "R", 60, "a5_album_6"},
    {"album", "B", 10, NULL},
    {"title", "RRR", 40, NULL},                       /* (back from the album on "Gioca") */
    {"title", "A", 60, "t6_menu_salvataggi_grandi"}, /* a tap: "questo è per mamma e papà", the gesture shown */
    {"title", "G", 10, NULL},                         /* L + R held: the grown-ups go in */
    {"salvataggi", "", 120, "s0_salvataggi"},
    {"salvataggi", "RA", 60, NULL},          /* a quick press: "questo è per mamma e papà" */
    {"salvataggi", "H", 10, NULL},           /* held: the keyboard opens */
    {"tastiera", "", 100, "k0_tastiera"},
    {"tastiera", "type:LUNA", 40, "k1_tastiera_luna"},
    {"tastiera", "type:!", 10, NULL},
    {"salvataggi", "", 110, "s1_salvataggi_luna"},
    {"salvataggi", "LY", 50, "s2_salvataggi_cancella"},
    {"salvataggi", "B", 20, NULL},
    {"salvataggi", "RY", 20, NULL},
    {"salvataggi", "Z", 60, "s3_salvataggi_ricomincia"},
    {"salvataggi", "LA", 10, NULL},
    {"title", "RRRG", 10, NULL},
    {"salvataggi", "RY", 20, NULL},
    {"salvataggi", "Z", 60, "s4_salvataggi_cancellato"},
    {"salvataggi", "B", 10, NULL},
    {"title", "R", 40, NULL},
    {"title", "A", 60, "t2_menu_grandi"},
    {"title", "G", 10, NULL},
    {"opzioni", "", 60, "o0_opzioni"},
    {"opzioni", "RR", 40, "o1_opzioni_musica"},
    {"opzioni", "DDDDDDDDDDDD", 20, NULL},
    {"opzioni", "A", 40, "o2_livelli"},
    {"opzioni", "R", 30, "o3_livelli_su"},
    {"opzioni", "B", 20, NULL},
    {"opzioni", "DA", 40, "o4_progressi"},
    {"opzioni", "B", 20, NULL},
    {"opzioni", "DA", 40, "o5_crediti"},
    {"opzioni", "B", 20, NULL},
    {"opzioni", "B", 10, NULL},
    {"title", "LLLLA", 10, NULL},
    {NULL, NULL, 0, NULL},
};
static const tstep_t TOUR_PAUSE[] = {
    {"*", "S", 70, "p0_pausa"},
    {"*", "R", 50, "p1_pausa_giochi"},
    {"*", "LA", 30, NULL},
    {NULL, NULL, 0, NULL},
};
static const tstep_t TOUR_EXIT[] = {
    {"menu", "S", 50, NULL},
    {"menu", "R", 50, "p2_pausa_menu"},
    {"menu", "A", 10, NULL},
    {"title", "RRRRR", 30, NULL},
    {"title", "A", 40, NULL},          /* a tap is not enough (0.14) */
    {"title", "G", 80, "t4_esci"},
    {"title", "L", 40, "t5_esci_nanna"},
    {"title", "A", 10, NULL},
    {NULL, NULL, 0, NULL},
};

/* 0.11: an adventure told again from the album */
static const tstep_t TOUR_RIVEDI[] = {
    {"title", "", 150, NULL},
    {"title", "RRA", 10, NULL},
    {"album", "", 100, "v0_album_storia"},
    {"album", "RRR", 90, "v1_album_storia4"},
    {"album", "A", 10, NULL},
    {"racconto", "", 240, "v2_rivedi_prologo"},
    {"album", "", 120, "v3_album_ritorno"},
    {"album", "B", 10, NULL},
    {"title", "LLA", 10, NULL}, /* and now play */
    {NULL, NULL, 0, NULL},
};

static bool want_tour, want_rivedi, tour_pause_done, tour_exit_started;

/* ------------------------------------------------------------------ the monkey (0.14) */
static bool monkey;
static unsigned long long mk_state;
enum { MK_IDLE, MK_PLAY, MK_MASH_A, MK_HOLD_A, MK_DPAD, MK_RANDOM, MK_START, MK_HOLD_ALL, MK_N };
static const char *MK_NAMES[MK_N] = {"idle", "play", "mash_a", "hold_a", "dpad", "random", "start", "hold_all"};
static int mk_mode = MK_IDLE;
static long mk_mode_until, mk_next, mk_counts[MK_N], mk_presses;

static uint32_t mk_rng(void)
{
    mk_state = mk_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (uint32_t)(mk_state >> 33);
}

static int mk_range(int a, int b) { return a + (int)(mk_rng() % (uint32_t)(b - a + 1)); }

static void mk_hold(uint32_t mask, int frames)
{
    press_mask = mask;
    press_until = frame_no + frames;
    mk_presses++;
}

static void monkey_step(void)
{
    static const unsigned DIRS[4] = {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
                                     RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT};
    static const unsigned ANY[12] = {RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X,
                                     RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_R,
                                     RETRO_DEVICE_ID_JOYPAD_START, RETRO_DEVICE_ID_JOYPAD_SELECT,
                                     RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_DOWN,
                                     RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_RIGHT};
    const uint32_t A = 1u << RETRO_DEVICE_ID_JOYPAD_A;
    int k;
    if (frame_no < press_until)
        return;
    press_mask = 0;
    if (frame_no >= mk_mode_until) { /* a new mood, for a few seconds */
        int r = mk_range(0, 99);
        mk_mode = r < 30 ? MK_PLAY : r < 45 ? MK_MASH_A : r < 55 ? MK_HOLD_A : r < 70 ? MK_DPAD
                : r < 85 ? MK_IDLE : r < 95 ? MK_RANDOM : r < 98 ? MK_START : MK_HOLD_ALL;
        mk_mode_until = frame_no + (mk_mode == MK_IDLE ? mk_range(120, 1200) : mk_range(60, 300));
        mk_counts[mk_mode]++;
        mk_next = frame_no;
    }
    if (frame_no < mk_next)
        return;
    switch (mk_mode) {
    case MK_IDLE: mk_next = mk_mode_until; break;
    case MK_PLAY: /* a look, a move or two, then the red button */
        if (mk_range(0, 2)) {
            mk_hold(1u << DIRS[mk_range(0, 3)], 3);
            mk_next = frame_no + mk_range(15, 45);
        } else {
            mk_hold(A, 4);
            mk_next = frame_no + mk_range(40, 150);
        }
        break;
    case MK_MASH_A: mk_hold(A, mk_range(2, 4)); mk_next = frame_no + mk_range(5, 12); break;
    case MK_HOLD_A: mk_hold(A, mk_range(30, 180)); mk_next = frame_no + mk_range(20, 60); break;
    case MK_DPAD: /* (one draw per statement: the order of a call's arguments is up to the compiler) */
        k = mk_range(0, 3);
        mk_hold(1u << DIRS[k], mk_range(2, 20));
        mk_next = frame_no + mk_range(8, 30);
        break;
    case MK_RANDOM:
        k = mk_range(0, 11);
        mk_hold(1u << ANY[k], mk_range(3, 10));
        mk_next = frame_no + mk_range(10, 40);
        break;
    case MK_START:
        mk_hold(1u << (mk_range(0, 1) ? RETRO_DEVICE_ID_JOYPAD_START : RETRO_DEVICE_ID_JOYPAD_SELECT), 3);
        mk_next = frame_no + mk_range(20, 60);
        break;
    default: /* everything at once */
        mk_hold(A | (1u << RETRO_DEVICE_ID_JOYPAD_B) | (1u << RETRO_DEVICE_ID_JOYPAD_X) |
                    (1u << RETRO_DEVICE_ID_JOYPAD_Y) | (1u << RETRO_DEVICE_ID_JOYPAD_START),
                mk_range(60, 150));
        mk_next = frame_no + mk_range(30, 90);
        break;
    }
}
static const tstep_t *tour;         /* running script, NULL = the normal bot plays */
static int tour_i, tour_k;          /* step, next key in it */
static long tour_shot_at = -1;

static void tour_start(const tstep_t *t)
{
    tour = t;
    tour_i = tour_k = 0;
    tour_shot_at = -1;
    tour_at = frame_no;
}

static unsigned key_id(char c)
{
    switch (c) {
    case 'L': return RETRO_DEVICE_ID_JOYPAD_LEFT;
    case 'R': return RETRO_DEVICE_ID_JOYPAD_RIGHT;
    case 'U': return RETRO_DEVICE_ID_JOYPAD_UP;
    case 'D': return RETRO_DEVICE_ID_JOYPAD_DOWN;
    case 'B': return RETRO_DEVICE_ID_JOYPAD_B;
    case 'X': return RETRO_DEVICE_ID_JOYPAD_X;
    case 'Y': return RETRO_DEVICE_ID_JOYPAD_Y;
    case 'S': return RETRO_DEVICE_ID_JOYPAD_START;
    case 'E': return RETRO_DEVICE_ID_JOYPAD_SELECT;
    default: return RETRO_DEVICE_ID_JOYPAD_A;
    }
}

/* one press (or one move on the keyboard) per call; true when the step's keys are done */
static bool tour_keys(const tstep_t *st)
{
    if (!strncmp(st->keys, "type:", 5)) {
        const char *word = st->keys + 5;
        if (tour_k >= (int)strlen(word))
            return true;
        char c = word[tour_k];
        int target = c == '!' ? 27 : (c == '<' ? 26 : c - 'A');
        int cr = tas_sel / 7, cc = tas_sel % 7, tr = target / 7, tc = target % 7;
        if (cc < tc)
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        else if (cc > tc)
            press(RETRO_DEVICE_ID_JOYPAD_LEFT);
        else if (cr < tr)
            press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        else if (cr > tr)
            press(RETRO_DEVICE_ID_JOYPAD_UP);
        else {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            tour_k++;
        }
        next_action = frame_no + 22;
        return false;
    }
    if (tour_k >= (int)strlen(st->keys))
        return true;
    char c = st->keys[tour_k++];
    if (c == 'H') { /* hold A: the grown-ups' door */
        press_mask = 1u << RETRO_DEVICE_ID_JOYPAD_A;
        press_until = frame_no + 135;
        next_action = frame_no + 150;
        if (!strcmp(scene, "title"))
            schedule_shot("t3_menu_tieni", 70);
        else if (!strcmp(scene, "salvataggi"))
            schedule_shot("s5_salvataggi_tieni", 70);
        return false;
    }
    if (c == 'G') { /* the grown-ups' gesture: L and R held together (0.14) */
        press_mask = (1u << RETRO_DEVICE_ID_JOYPAD_L) | (1u << RETRO_DEVICE_ID_JOYPAD_R);
        press_until = frame_no + 135;
        next_action = frame_no + 150;
        if (!strcmp(scene, "title"))
            schedule_shot("t3_menu_tieni", 70);
        return false;
    }
    if (c == 'S') { /* START held down: the pause (0.14: a tap does nothing) */
        press_mask = 1u << RETRO_DEVICE_ID_JOYPAD_START;
        press_until = frame_no + 50;
        next_action = frame_no + 64;
        return false;
    }
    if (c == 'Z') { /* hold X and Y together: delete a profile */
        press_mask = (1u << RETRO_DEVICE_ID_JOYPAD_X) | (1u << RETRO_DEVICE_ID_JOYPAD_Y);
        press_until = frame_no + 135;
        next_action = frame_no + 150;
        return false;
    }
    press(key_id(c));
    next_action = frame_no + 24;
    return false;
}

/* true while a script is running (the normal bot waits) */
static bool tour_step(void)
{
    if (!tour)
        return false;
    const tstep_t *st = &tour[tour_i];
    if (!st->scene) {
        tour = NULL;
        return false;
    }
    if (tour_shot_at >= 0) { /* waiting after the keys */
        if (frame_no < tour_shot_at)
            return true;
        if (st->shot && want_shots) {
            save_png(st->shot);
        }
        tour_shot_at = -1;
        tour_i++;
        tour_k = 0;
        return true;
    }
    if (tour_k == 0 && strcmp(st->scene, "*") && strcmp(st->scene, scene))
        return true; /* the scene has not arrived yet (its own keys may change it later) */
    if (tour_k == 0 && frame_no - tour_at < 40)
        return true; /* let a new scene settle */
    if (tour_keys(st))
        tour_shot_at = frame_no + st->wait;
    return true;
}

static unsigned wrong_dir(unsigned d)
{
    return d == RETRO_DEVICE_ID_JOYPAD_UP ? RETRO_DEVICE_ID_JOYPAD_LEFT : RETRO_DEVICE_ID_JOYPAD_UP;
}

/* --press-at F:K: a key pressed at a frame, over the bot (for tests of a single button) */
static struct {
    long at;
    char key;
} presses[64];
static int npresses;

static bool forced_press(void)
{
    for (int i = 0; i < npresses; i++)
        if (presses[i].at == frame_no) {
            unsigned k = presses[i].key == 'E' ? RETRO_DEVICE_ID_JOYPAD_SELECT : key_id(presses[i].key);
            press(k);
            next_action = frame_no + 30;
            fprintf(stderr, "%06ld forced press %c scene=%s\n", frame_no, presses[i].key, scene);
            return true;
        }
    return false;
}

static void bot_step(void)
{
    if (monkey) {
        monkey_step();
        return;
    }
    if (forced_press())
        return;
    if (frame_no < press_until)
        return;
    press_mask = 0;
    if (frame_no < next_action)
        return;
    if (want_rivedi) {
        static bool rivedi_started;
        if (!rivedi_started && !strcmp(scene, "title")) {
            rivedi_started = true;
            tour_start(TOUR_RIVEDI);
        }
        if (tour_step())
            return;
    }
    if (want_tour) {
        static bool main_started;
        if (!main_started && !strcmp(scene, "title")) {
            main_started = true;
            tour_start(TOUR_MAIN);
        }
        if (tour_step())
            return;
        if (!tour_pause_done && ready && game_index(scene) >= 0 && strcmp(scene, "balla")) {
            tour_pause_done = true; /* pause a question once: the game must wait, then go on */
            tour_start(TOUR_PAUSE);
            return;
        }
        if (!tour_exit_started && rewards >= 2 && !strcmp(scene, "menu") && frame_no - menu_at > 120) {
            tour_exit_started = true; /* and at the end, out through the main menu */
            tour_start(TOUR_EXIT);
            return;
        }
    }
    if (!strcmp(scene, "title")) {
        if (title_at >= 0 && frame_no - title_at > 240)
            press(RETRO_DEVICE_ID_JOYPAD_A);
    } else if (!strcmp(scene, "prova")) {
        /* a child tries the cross first, then finds the red button */
        if (prova_at < 0 || frame_no - prova_at < 170)
            return;
        if (prova_step == 'c')
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        else if (prova_step == 'r')
            press(RETRO_DEVICE_ID_JOYPAD_A);
        prova_at = -1;
    } else if (!strcmp(scene, "menu")) {
        if (menu_at < 0 || frame_no - menu_at < 200)
            return;
        /* entries: [the duel card when the wand is charged] + the open games + one locked "?" */
        bool here;
        if (menu_sfida) {
            here = menu_sel == 0;
            if (!here) {
                press(RETRO_DEVICE_ID_JOYPAD_LEFT);
                next_action = frame_no + 40;
                return;
            }
        } else {
            char want = games[menu_visits % strlen(games)];
            const char *k = strchr(GAME_KEYS, want);
            here = !strcmp(menu_game, GAME_NAMES[k ? (int)(k - GAME_KEYS) : G_CONTA]);
            if (here && menu_sel_locked) {
                menu_visits++; /* not open yet: the next game of the plan */
                menu_turns = 0;
                return;
            }
            if (!here) {
                if (menu_last_sel == menu_sel) { /* the end of the row: turn round */
                    menu_scan_dir = -menu_scan_dir;
                    if (++menu_turns >= 3) { /* not in the menu: the next game of the plan */
                        menu_visits++;
                        menu_turns = 0;
                    }
                }
                menu_last_sel = menu_sel;
                press(menu_scan_dir > 0 ? RETRO_DEVICE_ID_JOYPAD_RIGHT : RETRO_DEVICE_ID_JOYPAD_LEFT);
                next_action = frame_no + 40;
                return;
            }
        }
        /* keep pressing until the scene changes: a press during the
           unlock reveal or a voice line may be ignored */
        press(RETRO_DEVICE_ID_JOYPAD_A);
        next_action = frame_no + 70;
    } else if (!strcmp(scene, "racconto")) {
        /* the kind act of an ending: her turn (the cross for the dance, else the red button) */
        if (!act_kind[0] || act_next < 0 || frame_no < act_next)
            return;
        if (!strcmp(act_kind, "dance")) {
            static const unsigned STEPS[4] = {RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_RIGHT,
                                              RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT};
            press(STEPS[act_presses % 4]);
            next_action = act_next = frame_no + 34;
        } else if (!strcmp(act_kind, "ball")) {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            /* her throw; the first time a second press right after it, as children do: "aspetta!" */
            act_next = act_ball_mine && !act_wait_tested ? frame_no + 14 : -1;
            if (act_ball_mine)
                act_wait_tested = true;
            act_ball_mine = false;
        } else {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            next_action = act_next = frame_no + (!strcmp(act_kind, "lantern") ? 40 : 28);
        }
        act_presses++;
    } else if (!strcmp(scene, "mappa")) {
        if (map_ready) { /* again every so often, until the map is left */
            press(RETRO_DEVICE_ID_JOYPAD_A);
            next_action = frame_no + 90;
        }
    } else if (!strcmp(scene, "memory")) {
        if (!mem_ready || frame_no < mem_wait || mem_n == 0)
            return;
        /* choose the card to turn: the first unfound one, then its partner
           (or, when the plan wants a mistake, a card that does not match) */
        int target = -1;
        if (mem_first < 0) {
            for (int i = 0; i < mem_n; i++)
                if (!mem_found[i]) {
                    target = i;
                    break;
                }
        } else {
            char want = plan_now();
            int need_wrong = want == 'w' ? 1 : (want == 'h' ? 3 : 0);
            for (int i = 0; i < mem_n && target < 0; i++) {
                if (i == mem_first || mem_found[i])
                    continue;
                bool partner = mem_item[i] == mem_item[mem_first];
                if (mem_misses_pair < need_wrong ? !partner : partner)
                    target = i;
            }
            if (target < 0)
                for (int i = 0; i < mem_n; i++)
                    if (i != mem_first && !mem_found[i] && mem_item[i] == mem_item[mem_first])
                        target = i;
        }
        if (target < 0)
            return;
        int cr = mem_cur / mem_cols, cc = mem_cur % mem_cols, tr = target / mem_cols, tc = target % mem_cols;
        if (cc < tc)
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        else if (cc > tc)
            press(RETRO_DEVICE_ID_JOYPAD_LEFT);
        else if (cr < tr)
            press(RETRO_DEVICE_ID_JOYPAD_DOWN);
        else if (cr > tr)
            press(RETRO_DEVICE_ID_JOYPAD_UP);
        else {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            if (mem_first < 0) {
                mem_first = target;
            } else {
                if (mem_item[target] == mem_item[mem_first])
                    mem_found[target] = mem_found[mem_first] = true;
                mem_first = -1;
            }
            next_action = frame_no + 24;
        }
    } else if (!strcmp(scene, "ritmo")) {
        if (rit_start < 0 || frame_no < rit_start || rit_k >= rit_n)
            return;
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        bool wrong = rit_turns <= need_wrong;
        int n = wrong ? (rit_n > 2 ? rit_n - 1 : rit_n + 1) : rit_n; /* a wrong try: one clap too few or too many */
        if (rit_k >= n) {
            rit_start = -1;
            return;
        }
        long when = rit_start + (wrong ? rit_k * 20 : rit_at[rit_k]) + (rit_k % 2);
        if (frame_no >= when) {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            next_action = frame_no + 4;
            rit_k++;
        }
    } else if (!strcmp(scene, "dove")) {
        if (!dove_ready)
            return;
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        char ch = attempts < need_wrong && dove_other != '?' ? dove_other : dove_goal;
        switch (ch) {
        case 'U': press(RETRO_DEVICE_ID_JOYPAD_UP); break;
        case 'D': press(RETRO_DEVICE_ID_JOYPAD_DOWN); break;
        case 'L': press(RETRO_DEVICE_ID_JOYPAD_LEFT); break;
        case 'R': press(RETRO_DEVICE_ID_JOYPAD_RIGHT); break;
        case '-':
            press(RETRO_DEVICE_ID_JOYPAD_A);
            dove_ready = false;
            break;
        default: return;
        }
        next_action = frame_no + 24;
    } else if (!strcmp(scene, "ginnastica")) {
        if (!gi_ready || gi_i >= gi_len)
            return;
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        unsigned d = gi_seq[gi_i];
        if (!gi_guided && attempts < need_wrong && gi_i == gi_len - 1)
            d = gi_len > 1 && gi_seq[gi_len - 2] == RETRO_DEVICE_ID_JOYPAD_A ? RETRO_DEVICE_ID_JOYPAD_A
                                                                               : RETRO_DEVICE_ID_JOYPAD_UP;
        if (!gi_guided && attempts < need_wrong && gi_i == gi_len - 1 && d == RETRO_DEVICE_ID_JOYPAD_UP)
            d = RETRO_DEVICE_ID_JOYPAD_DOWN; /* no jump before the salute: another move */
        press(d);
        gi_i++;
        next_action = frame_no + 44;
    } else if (!strcmp(scene, "sentiero")) {
        if (!sen_ready) {
            if (sen_wait_until < 0 || frame_no < sen_wait_until)
                return;
            sen_ready = true; /* the press was lost (or refused): look again */
            sen_wait_until = -1;
        }
        size_t nc = strlen(sen_cur), nw = strlen(sen_want);
        if (nc > nw || strncmp(sen_cur, sen_want, nc)) { /* fix the row: the yellow button takes one away */
            press(RETRO_DEVICE_ID_JOYPAD_B);
        } else if (nc < nw) {
            char c = sen_want[nc];
            press(c == 'U' ? RETRO_DEVICE_ID_JOYPAD_UP : c == 'D' ? RETRO_DEVICE_ID_JOYPAD_DOWN
                  : c == 'L' ? RETRO_DEVICE_ID_JOYPAD_LEFT : RETRO_DEVICE_ID_JOYPAD_RIGHT);
            next_action = frame_no + 22;
        } else {
            press(RETRO_DEVICE_ID_JOYPAD_A); /* go, Deva! */
        }
        sen_ready = false; /* until the core echoes the key ("sentiero add/del/run") */
        sen_wait_until = frame_no + 240;
    } else if (!strcmp(scene, "balla")) {
        if (!balla_ready || balla_i >= balla_len)
            return;
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        unsigned d = balla_seq[balla_i];
        if (!balla_guided && attempts < need_wrong && balla_i == balla_len - 1)
            d = wrong_dir(d); /* get the last move wrong */
        press(d);
        balla_i++;
        next_action = frame_no + 24;
    } else if ((game_index(scene) >= 0 || !strcmp(scene, "sfida")) && ready && correct_idx >= 0) {
        char want = plan_now();
        int need_wrong = want == 'w' ? 1 : (want == 'h' ? 2 : 0);
        int target = correct_idx;
        if (attempts < need_wrong)
            for (int i = 0; i < 3; i++)
                if (i != correct_idx && !off[i]) {
                    target = i;
                    break;
                }
        if (sel != target && passi_key[target])
            press(passi_key[target]);
        else if (sel < target)
            press(RETRO_DEVICE_ID_JOYPAD_RIGHT);
        else if (sel > target)
            press(RETRO_DEVICE_ID_JOYPAD_LEFT);
        else {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            ready = false;
        }
    } else if (!strcmp(scene, "premio")) {
        if (!waiting_reward_continue && offer_at && frame_no - offer_at > 240) {
            /* hover right (hear the name), then back to the first card - always a
               new item while any is left - and take it */
            static long last_offer_handled;
            static int step;
            if (last_offer_handled != offer_at) {
                last_offer_handled = offer_at;
                step = 0;
            }
            static const unsigned KEYS[4] = {RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_LEFT,
                                             RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_A};
            press(KEYS[step < 3 ? step : 3]);
            if (step < 3)
                next_action = frame_no + 50;
            step++;
        } else if (waiting_reward_continue && frame_no - reward_at > 420) {
            press(RETRO_DEVICE_ID_JOYPAD_A);
            waiting_reward_continue = false;
        }
    }
}

/* ------------------------------------------------------------------ callbacks */

static void core_log(enum retro_log_level level, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (log_file)
        fprintf(log_file, "%06ld %s", frame_no, buf);
    if (level == RETRO_LOG_WARN)
        warnings++;
    if (level == RETRO_LOG_ERROR)
        errors++;
    if (level >= RETRO_LOG_WARN)
        fprintf(stderr, "%06ld %s", frame_no, buf);
    const char *b = strstr(buf, "BOT ");
    if (b)
        on_bot(b + 4);
}

static bool rumble_state(unsigned port, enum retro_rumble_effect e, uint16_t s)
{
    (void)port;
    (void)e;
    if (s)
        rumbles++;
    return true;
}

static bool env(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log = core_log;
        return true;
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char **)data = sys_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = save_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true;
        return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        return true;
    case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        ((struct retro_rumble_interface *)data)->set_rumble_state = rumble_state;
        return true;
    case RETRO_ENVIRONMENT_SHUTDOWN:
        shutdown_req = true;
        return true;
    default:
        return false;
    }
}

static void save_png(const char *name)
{
    static unsigned char rgb[W * H * 3];
    for (int i = 0; i < W * H; i++) {
        uint16_t p = frame_buf[i];
        rgb[3 * i] = (unsigned char)(((p >> 11) & 31) * 255 / 31);
        rgb[3 * i + 1] = (unsigned char)(((p >> 5) & 63) * 255 / 63);
        rgb[3 * i + 2] = (unsigned char)((p & 31) * 255 / 31);
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/%.64s.png", out_dir, name);
    stbi_write_png(path, W, H, 3, rgb, W * 3);
}

/* --digest (1.0): one hash of every new picture (and when it came) and one of all the sound, to prove
   that an optimization changes nothing a child could see or hear */
static bool want_digest;
static uint64_t dig_video = 1469598103934665603ULL, dig_audio = 1469598103934665603ULL;
static FILE *frame_hashes; /* --frame-hashes FILE: "frame hash" of every new picture, to find where two runs part */

static uint64_t dig_words(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        uint64_t w;
        memcpy(&w, b + i, 8);
        h = (h ^ w) * 1099511628211ULL;
        h ^= h >> 29;
    }
    for (; i < n; i++)
        h = (h ^ b[i]) * 1099511628211ULL;
    return h;
}

static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
    if (!data) {
        dupes++;
    } else {
        fresh++;
        for (unsigned y = 0; y < h && y < H; y++)
            memcpy(frame_buf + y * W, (const char *)data + y * pitch, (w < W ? w : W) * 2);
        if (want_digest) {
            dig_video = dig_words(dig_video, &frame_no, sizeof(frame_no));
            dig_video = dig_words(dig_video, frame_buf, (size_t)W * H * 2);
        }
        if (frame_hashes)
            fprintf(frame_hashes, "%ld %016llx\n", frame_no,
                    (unsigned long long)dig_words(1469598103934665603ULL, frame_buf, (size_t)W * H * 2));
    }
    if (video_pipe)
        fwrite(frame_buf, 2, W * H, video_pipe);
}

static void audio_sample(int16_t l, int16_t r)
{
    (void)l;
    (void)r;
}

static size_t audio_batch(const int16_t *data, size_t frames)
{
    audio_frames += (long)frames;
    if (want_digest)
        dig_audio = dig_words(dig_audio, data, frames * 4);
    if (audio_raw)
        fwrite(data, 4, frames, audio_raw);
    return frames;
}

static void input_poll(void) {}

static int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port || device != RETRO_DEVICE_JOYPAD || index)
        return 0;
    uint32_t bits = press_mask;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
        return (int16_t)(frame_no < press_until ? bits : 0);
    return (int16_t)(frame_no < press_until && (bits >> id) & 1);
}

static void *sym(void *h, const char *n)
{
    void *p = dlsym(h, n);
    if (!p) {
        fprintf(stderr, "missing symbol %s\n", n);
        exit(2);
    }
    return p;
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s core.so system_dir save_dir out_dir [--plan cwh] [--games cpsb] [--frames N] "
                        "[--shots] [--shot-at F:NAME] [--press-at F:KEY] [--video out.mp4] [--audio out.raw] "
                        "[--tour] [--rivedi] [--idle-acts] [--monkey SEED]\n", argv[0]);
        return 2;
    }
    sys_dir = argv[2];
    save_dir = argv[3];
    out_dir = argv[4];
    long max_frames = 60000;
    const char *video_out = NULL, *audio_out = NULL;
    for (int i = 5; i < argc; i++) {
        if (!strcmp(argv[i], "--plan") && i + 1 < argc)
            plan = argv[++i];
        else if (!strcmp(argv[i], "--games") && i + 1 < argc)
            games = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            max_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--shots"))
            want_shots = true;
        else if (!strcmp(argv[i], "--digest"))
            want_digest = true;
        else if (!strcmp(argv[i], "--frame-hashes") && i + 1 < argc) {
            frame_hashes = fopen(argv[++i], "w");
            if (!frame_hashes)
                fprintf(stderr, "cannot write %s\n", argv[i]);
        }
        else if (!strcmp(argv[i], "--shot-at") && i + 1 < argc) {
            const char *a = argv[++i], *c = strchr(a, ':');
            if (c && c[1] && nshots < 256) {
                shots[nshots].at = atol(a);
                snprintf(shots[nshots].name, sizeof(shots[0].name), "%s", c + 1);
                nshots++;
            }
        }
        else if (!strcmp(argv[i], "--press-at") && i + 1 < argc) {
            const char *a = argv[++i], *c = strchr(a, ':');
            if (c && c[1] && npresses < 64) {
                presses[npresses].at = atol(a);
                presses[npresses].key = c[1];
                npresses++;
            }
        }
        else if (!strcmp(argv[i], "--tour"))
            want_tour = true;
        else if (!strcmp(argv[i], "--rivedi"))
            want_rivedi = true;
        else if (!strcmp(argv[i], "--idle-acts")) /* leave the kind acts of the endings to themselves */
            idle_acts = true;
        else if (!strcmp(argv[i], "--video") && i + 1 < argc)
            video_out = argv[++i];
        else if (!strcmp(argv[i], "--audio") && i + 1 < argc)
            audio_out = argv[++i]; /* raw s16le stereo 44.1 kHz, for regression checks */
        else if (!strcmp(argv[i], "--monkey") && i + 1 < argc) {
            monkey = true;
            mk_state = strtoull(argv[++i], NULL, 10) * 2654435761ULL + 1;
        }
    }
    char path[600];
    snprintf(path, sizeof(path), "%s/log.txt", out_dir);
    log_file = fopen(path, "w");

    void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 2;
    }
    *(void **)&core.init = sym(h, "retro_init");
    *(void **)&core.deinit = sym(h, "retro_deinit");
    *(void **)&core.set_environment = sym(h, "retro_set_environment");
    *(void **)&core.set_video_refresh = sym(h, "retro_set_video_refresh");
    *(void **)&core.set_audio_sample = sym(h, "retro_set_audio_sample");
    *(void **)&core.set_audio_sample_batch = sym(h, "retro_set_audio_sample_batch");
    *(void **)&core.set_input_poll = sym(h, "retro_set_input_poll");
    *(void **)&core.set_input_state = sym(h, "retro_set_input_state");
    *(void **)&core.get_system_av_info = sym(h, "retro_get_system_av_info");
    *(void **)&core.load_game = sym(h, "retro_load_game");
    *(void **)&core.unload_game = sym(h, "retro_unload_game");
    *(void **)&core.run = sym(h, "retro_run");

    core.set_environment(env);
    core.init();
    core.set_video_refresh(video);
    core.set_audio_sample(audio_sample);
    core.set_audio_sample_batch(audio_batch);
    core.set_input_poll(input_poll);
    core.set_input_state(input_state);
    struct retro_system_av_info av;
    core.get_system_av_info(&av);
    struct timespec l0, l1;
    clock_gettime(CLOCK_MONOTONIC, &l0);
    if (!core.load_game(NULL)) {
        fprintf(stderr, "retro_load_game failed\n");
        return 3;
    }
    clock_gettime(CLOCK_MONOTONIC, &l1);
    double load_ms = (l1.tv_sec - l0.tv_sec) * 1e3 + (l1.tv_nsec - l0.tv_nsec) / 1e6;
    if (audio_out && !video_out)
        audio_raw = fopen(audio_out, "wb");
    char tmp_video[600], raw_audio[600];
    if (video_out) {
        snprintf(tmp_video, sizeof(tmp_video), "%s/.video.mp4", out_dir);
        snprintf(raw_audio, sizeof(raw_audio), "%s/.audio.raw", out_dir);
        char cmd[1400];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -v error -y -f rawvideo -pix_fmt rgb565le -s %dx%d -r 60 -i - "
                 "-vf scale=640:480:flags=neighbor,fps=30 -c:v libx264 -preset veryfast -crf 20 "
                 "-pix_fmt yuv420p '%s'",
                 W, H, tmp_video);
        video_pipe = popen(cmd, "w");
        audio_raw = fopen(raw_audio, "wb");
    }

    struct timespec t0, t1;
    const char *slow_env = getenv("DEVA_SLOW_MS"); /* report frames slower than this */
    double slow_us = slow_env ? atof(slow_env) * 1000.0 : 0;
    double total_us = 0, max_us = 0;
    long max_at = 0;
    double max_late_us = 0;
    for (frame_no = 0; frame_no < max_frames && !shutdown_req; frame_no++) {
        bot_step();
        clock_gettime(CLOCK_MONOTONIC, &t0);
        core.run();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double us = (t1.tv_sec - t0.tv_sec) * 1e6 + (t1.tv_nsec - t0.tv_nsec) / 1e3;
        total_us += us;
        if (us > max_us && frame_no > 5) {
            max_us = us;
            max_at = frame_no;
        }
        if (us > max_late_us && frame_no > 600) /* after the background decoding of the clips */
            max_late_us = us;
        if (frame_no < MAX_TIMES)
            times[frame_no] = (float)us;
        if (slow_us > 0 && us > slow_us && frame_no > 5)
            fprintf(stderr, "slow frame %ld: %.1f ms in %s\n", frame_no, us / 1000.0, scene);
        for (int i = 0; i < nshots; i++)
            if (shots[i].at == frame_no) {
                save_png(shots[i].name);
            }
    }
    core.unload_game();
    core.deinit();
    if (video_pipe) {
        pclose(video_pipe);
        fclose(audio_raw);
        char cmd[4096];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -v error -y -i '%s' -f s16le -ar 44100 -ac 2 -i '%s' -c:v copy -c:a aac -b:a 128k "
                 "-shortest '%s' && rm -f '%s' '%s'",
                 tmp_video, raw_audio, video_out, tmp_video, raw_audio);
        if (system(cmd) != 0)
            fprintf(stderr, "ffmpeg mux failed\n");
    }
    if (log_file)
        fclose(log_file);
    if (audio_out && !video_out && audio_raw)
        fclose(audio_raw);

    printf("frames=%ld (%.1f s) fresh=%ld dupes=%ld audio_ok=%d\n", frame_no, frame_no / 60.0, fresh, dupes,
           audio_frames == frame_no * 735);
    if (want_digest)
        printf("digest video=%016llx audio=%016llx\n", (unsigned long long)dig_video, (unsigned long long)dig_audio);
    printf("questions=%d answers=%d wrong=%d hints=%d rewards=%d level_ups=%d level_downs=%d unlocks=%d\n",
           question_no, answers, wrongs, hints, rewards, level_ups, level_downs, unlocks);
    for (int g = 0; g < G_N; g++)
        printf("  %-9s rounds=%d questions=%d max_level=%d\n", GAME_NAMES[g], game_rounds[g], game_q[g],
               game_maxlvl[g]);
    printf("story: map_visits=%d duels=%d won=%d finale=%d recaps=%d epilogue=%d\n", map_visits, duels, duels_won,
           story_done, recaps, epilogue_done);
    printf("acts: done=%d presses=%d auto=%d waits=%d\n", acts_done, act_presses, acts_auto, act_waits);
    if (monkey) {
        printf("monkey: presses=%ld", mk_presses);
        for (int m = 0; m < MK_N; m++)
            printf(" %s=%ld", MK_NAMES[m], mk_counts[m]);
        printf(" quit_at=%ld\n", shutdown_req ? frame_no : -1L);
    }
    if (want_tour)
        printf("tour: steps_left=%s pauses=%d new_profiles=%d deleted=%d exit=%d\n", tour ? "yes" : "no", pauses,
               profiles_new, profiles_deleted, tour_exit_started);
    {
        /* memory of the whole process (harness + core) and load time */
        FILE *st = fopen("/proc/self/status", "r");
        char line[256];
        long rss = 0, hwm = 0;
        while (st && fgets(line, sizeof(line), st)) {
            if (!strncmp(line, "VmRSS:", 6))
                rss = atol(line + 6);
            if (!strncmp(line, "VmHWM:", 6))
                hwm = atol(line + 6);
        }
        if (st)
            fclose(st);
        printf("memory: rss=%ld KB peak=%ld KB, retro_load_game=%.1f ms\n", rss, hwm, load_ms);
    }
    printf("rumble=%ld shutdown=%d warnings=%ld errors=%ld\n", rumbles, shutdown_req, warnings, errors);
    printf("retro_run avg=%.0f us max=%.0f us at frame %ld, max after 10 s=%.0f us (host CPU)\n",
           total_us / (frame_no ? frame_no : 1), max_us, max_at, max_late_us);
    long nt = frame_no < MAX_TIMES ? frame_no : MAX_TIMES, over1 = 0;
    for (long i = 0; i < nt; i++)
        over1 += times[i] > 1000.0f;
    qsort(times, (size_t)nt, sizeof(float), cmp_float);
    if (nt > 0)
        printf("retro_run p50=%.0f us p99=%.0f us p99.9=%.0f us, frames over 1 ms: %ld\n", times[nt / 2],
               times[nt * 99 / 100], times[nt * 999 / 1000], over1);
    return errors ? 1 : 0;
}
