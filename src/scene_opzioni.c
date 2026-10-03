/* Deva's Awesome Adventures - options, for the grown-ups (opened by holding A
 * in the main menu). A list: volumes (heard at once), rumble, play time,
 * questions per round, the tale, unlocking, levels of each game, progress of
 * the profile, credits, reset. Saved on leaving, in the save directory
 * (deva_adventures_opzioni.cfg: only what differs from deva_adventures.cfg).
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "story.h"
#include "ui.h"

enum { PG_MAIN, PG_LIVELLI, PG_PROGRESSI, PG_CREDITI };

enum {
    OP_MUSIC, OP_VOICE, OP_SFX, OP_RUMBLE, OP_ANIM, OP_SESSION, OP_QUESTIONS, OP_STORY, OP_DUEL, OP_UNLOCK,
    OP_UNLOCK_ROUNDS, OP_START_LEVEL, OP_LEVELS, OP_PROGRESS, OP_CREDITS, OP_RESET, OP_BACK, OP_COUNT
};

static const char *LABELS[OP_COUNT] = {
    "Volume musica",      "Volume voce",       "Volume effetti",
    "Vibrazione",         "Animazioni",        "Tempo di gioco",
    "Domande per round",  "Avventure (mappa e sfide)", "Round per caricare la bacchetta",
    "Giochi nuovi",       "Round per un gioco nuovo", "Livello iniziale di Conta",
    "Livelli dei giochi", "Progressi",         "Crediti",
    "Ripristina le opzioni", "Esci e salva",
};
static const char *IDS[OP_COUNT] = {"musica",  "voce",    "effetti",          "vibrazione", "animazioni", "sessione",
                                    "domande", "storia",  "sfida",            "sblocco",    "round_sblocco",
                                    "livello_iniziale",   "livelli", "progressi", "crediti", "ripristina", "esci"};
static const int SESSION_STEPS[] = {0, 5, 10, 15, 20, 30, 45, 60, 90, 120};

#define ROW_H 12
#define LIST_Y 42
#define VISIBLE 14

static struct {
    int page, sel, top, t, hold;
    int lsel;          /* Livelli: the game highlighted */
    bool dirty_prog;   /* levels changed: save the profile */
} O;

static bool enabled(int row) { return row != OP_UNLOCK_ROUNDS || G.cfg.unlock_gradual; }

static void value_text(int row, char *out, size_t n)
{
    const config_t *c = &G.cfg;
    switch (row) {
    case OP_MUSIC: snprintf(out, n, "%d%%", c->vol_music); break;
    case OP_VOICE: snprintf(out, n, "%d%%", c->vol_voice); break;
    case OP_SFX: snprintf(out, n, "%d%%", c->vol_sfx); break;
    case OP_RUMBLE: snprintf(out, n, "%s", c->rumble ? "sì" : "no"); break;
    case OP_ANIM: snprintf(out, n, "%s", c->animations ? "sì" : "no"); break;
    case OP_SESSION:
        if (c->session_minutes > 0)
            snprintf(out, n, "%d minuti", c->session_minutes);
        else
            snprintf(out, n, "senza limite");
        break;
    case OP_QUESTIONS: snprintf(out, n, "%d", c->questions_per_round); break;
    case OP_STORY: snprintf(out, n, "%s", c->story ? "sì" : "no"); break;
    case OP_DUEL: snprintf(out, n, "%d", c->rounds_per_duel); break;
    case OP_UNLOCK: snprintf(out, n, "%s", c->unlock_gradual ? "uno alla volta" : "tutti subito"); break;
    case OP_UNLOCK_ROUNDS: snprintf(out, n, "%d", c->rounds_per_unlock); break;
    case OP_START_LEVEL: snprintf(out, n, "%d (nuovi profili)", c->start_level); break;
    case OP_LEVELS: case OP_PROGRESS: case OP_CREDITS: snprintf(out, n, ">"); break;
    case OP_RESET: snprintf(out, n, "tieni premuto A"); break;
    default: out[0] = 0; break;
    }
}

static bool adjustable(int row) { return row <= OP_START_LEVEL; }

static void report(int row)
{
    char v[40];
    value_text(row, v, sizeof(v));
    BOT("opzioni row=%s value=%s\n", IDS[row], v);
}

static int step_session(int cur, int dir)
{
    int n = ARRAY_LEN(SESSION_STEPS);
    if (dir > 0) {
        for (int i = 0; i < n; i++)
            if (SESSION_STEPS[i] > cur)
                return SESSION_STEPS[i];
        return SESSION_STEPS[n - 1];
    }
    for (int i = n - 1; i >= 0; i--)
        if (SESSION_STEPS[i] < cur)
            return SESSION_STEPS[i];
    return SESSION_STEPS[0];
}

static void adjust(int row, int dir)
{
    config_t *c = &G.cfg;
    if (!enabled(row)) {
        sfx("boop");
        return;
    }
    switch (row) {
    case OP_MUSIC: c->vol_music = clampi(c->vol_music + dir * 5, 0, 100); break;
    case OP_VOICE: c->vol_voice = clampi(c->vol_voice + dir * 5, 0, 100); break;
    case OP_SFX: c->vol_sfx = clampi(c->vol_sfx + dir * 5, 0, 100); break;
    case OP_RUMBLE: c->rumble = !c->rumble; break;
    case OP_ANIM: c->animations = !c->animations; break;
    case OP_SESSION: c->session_minutes = step_session(c->session_minutes, dir); break;
    case OP_QUESTIONS: c->questions_per_round = clampi(c->questions_per_round + dir, 1, 10); break;
    case OP_STORY: c->story = !c->story; break;
    case OP_DUEL: c->rounds_per_duel = clampi(c->rounds_per_duel + dir, 1, 5); break;
    case OP_UNLOCK: c->unlock_gradual = !c->unlock_gradual; break;
    case OP_UNLOCK_ROUNDS: c->rounds_per_unlock = clampi(c->rounds_per_unlock + dir, 1, 20); break;
    case OP_START_LEVEL: c->start_level = clampi(c->start_level + dir, 1, 5); break;
    default: return;
    }
    audio_set_volumes(c->vol_music, c->vol_voice, c->vol_sfx);
    /* hear (or feel) the new value */
    if (row == OP_VOICE)
        say("m_gioca");
    else if (row == OP_SFX)
        sfx("star");
    else if (row == OP_RUMBLE && c->rumble)
        frontend_rumble(1, 15);
    else if (row != OP_MUSIC)
        sfx("blip");
    report(row);
}

static void open_page(int p)
{
    O.page = p;
    O.t = 0;
    O.hold = 0;
    sfx("blip");
    BOT("opzioni page=%d\n", p);
}

static void leave(void)
{
    game_save_options();
    if (O.dirty_prog)
        game_save();
    audio_set_volumes(G.cfg.vol_music, G.cfg.vol_voice, G.cfg.vol_sfx);
    sfx("blip");
    BOT("opzioni saved\n");
    game_goto(SC_TITLE);
}

static void enter(void)
{
    memset(&O, 0, sizeof(O));
    music_play(snd_find(SND_MUSIC, "palco"));
    BOT("opzioni sel=0\n");
}

static void move_sel(int dir)
{
    int to = O.sel + dir;
    if (to < 0 || to >= OP_COUNT)
        return;
    O.sel = to;
    O.hold = 0;
    if (O.sel < O.top)
        O.top = O.sel;
    if (O.sel >= O.top + VISIBLE)
        O.top = O.sel - VISIBLE + 1;
    sfx("blip");
    BOT("opzioni sel=%d row=%s\n", O.sel, IDS[O.sel]);
}

static void update_main(void)
{
    if (O.sel == OP_RESET && O.hold > 0) {
        if (btn_held(BTN_A)) {
            if (++O.hold >= HOLD_FRAMES) {
                O.hold = 0;
                G.cfg = G.cfg_base;
                audio_set_volumes(G.cfg.vol_music, G.cfg.vol_voice, G.cfg.vol_sfx);
                sfx("sparkle");
                BOT("opzioni reset\n");
            }
        } else {
            O.hold = 0;
        }
        return;
    }
    int dy = btn_pressed(BTN_UP) ? -1 : (btn_pressed(BTN_DOWN) ? 1 : 0);
    int dx = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dy) {
        move_sel(dy);
    } else if (dx && adjustable(O.sel)) {
        adjust(O.sel, dx);
    } else if (btn_pressed(BTN_A)) {
        switch (O.sel) {
        case OP_LEVELS: open_page(PG_LIVELLI); break;
        case OP_PROGRESS: open_page(PG_PROGRESSI); break;
        case OP_CREDITS: open_page(PG_CREDITI); break;
        case OP_RESET: O.hold = 1; break;
        case OP_BACK: leave(); break;
        case OP_RUMBLE: case OP_STORY: case OP_UNLOCK: adjust(O.sel, 1); break;
        default: break;
        }
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_START)) {
        leave();
    }
}

static void update_livelli(void)
{
    int dy = btn_pressed(BTN_UP) ? -1 : (btn_pressed(BTN_DOWN) ? 1 : 0);
    int dx = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dy && O.lsel + dy >= 0 && O.lsel + dy < GAME_COUNT) {
        O.lsel += dy;
        sfx("blip");
    } else if (dx) {
        int *lvl = &G.prog.level[O.lsel];
        int v = clampi(*lvl + dx, 1, 5);
        if (v != *lvl) {
            *lvl = v;
            G.prog.streak[O.lsel] = 0;
            O.dirty_prog = true;
            sfx("star");
            BOT("opzioni level game=%s level=%d\n", GAME_IDS[O.lsel], v);
        } else {
            sfx("boop");
        }
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_A) || btn_pressed(BTN_START)) {
        open_page(PG_MAIN);
    }
}

static void update(void)
{
    O.t++;
    switch (O.page) {
    case PG_MAIN: update_main(); break;
    case PG_LIVELLI: update_livelli(); break;
    default:
        if (btn_pressed(BTN_B) || btn_pressed(BTN_A) || btn_pressed(BTN_START))
            open_page(PG_MAIN);
        break;
    }
}

/* ------------------------------------------------------------------ drawing */

static void draw_main(void)
{
    ui_title("OPZIONI", 16);
    ui_panel(8, 32, 304, 186);
    char v[48], line[64];
    for (int i = O.top; i < OP_COUNT && i < O.top + VISIBLE; i++) {
        int y = LIST_Y + (i - O.top) * ROW_H;
        bool sel = i == O.sel;
        if (sel)
            gfx_round_rect(12, y - 2, 296, ROW_H + 1, COL_PINK_L, COL_HOT);
        uint16_t ink = enabled(i) ? COL_INK : COL_GREY;
        gfx_text(LABELS[i], 20, y, ink, ALIGN_LEFT);
        value_text(i, v, sizeof(v));
        if (sel && adjustable(i) && enabled(i))
            snprintf(line, sizeof(line), "< %s >", v);
        else
            snprintf(line, sizeof(line), "%s", v);
        gfx_text(line, 300, y, sel ? COL_HOT : (enabled(i) ? COL_LILAC_D : COL_GREY), ALIGN_RIGHT);
        if (sel && i == OP_RESET && O.hold > 0)
            ui_bar(20, y + 9, 280, O.hold, HOLD_FRAMES);
    }
    /* more rows below / above: small triangles on the right edge */
    for (int k = 0; k < 3; k++) {
        if (O.top > 0)
            gfx_fill_rect(304 - k, 35 + k, 1 + 2 * k, 1, COL_HOT);
        if (O.top + VISIBLE < OP_COUNT)
            gfx_fill_rect(302 + k, 211 + k, 5 - 2 * k, 1, COL_HOT);
    }
    if (adjustable(O.sel))
        ui_footer("Su/giù: scegli   Sinistra/destra: cambia   B: esci e salva");
    else
        ui_footer("Su/giù: scegli   A: apri   B: esci e salva");
}

static void draw_livelli(void)
{
    ui_title("LIVELLI", 16);
    ui_panel(8, 32, 304, 186);
    char line[64];
    snprintf(line, sizeof(line), "Profilo %d: %s  -  sale da solo, 3 giuste di fila", G.slot, child_name());
    gfx_text(line, 160, 38, COL_LILAC_D, ALIGN_CENTER);
    for (int g = 0; g < GAME_COUNT; g++) { /* 16 games: rows of 10 px, the level as five little squares */
        int y = 50 + g * 10;
        bool sel = g == O.lsel;
        if (sel)
            gfx_round_rect(12, y - 1, 296, 11, COL_PINK_L, COL_HOT);
        gfx_text(GAME_TITLES[g], 22, y, COL_INK, ALIGN_LEFT);
        int lvl = clampi(G.prog.level[g], 1, 5);
        for (int k = 0; k < 5; k++)
            gfx_round_rect(196 + k * 11, y + 1, 8, 7, k < lvl ? COL_GOLD : COL_CREAM, k < lvl ? COL_INK : COL_LILAC);
        snprintf(line, sizeof(line), sel ? "< %d >" : "%d", lvl);
        gfx_text(line, 300, y, sel ? COL_HOT : COL_LILAC_D, ALIGN_RIGHT);
    }
    ui_footer("Su/giù: scegli il gioco   Sinistra/destra: livello   B: indietro");
}

static void draw_progressi(void)
{
    ui_title("PROGRESSI", 16);
    ui_panel(8, 32, 304, 186);
    const progress_t *p = &G.prog;
    char line[96];
    snprintf(line, sizeof(line), "Profilo %d: %s", G.slot, child_name());
    gfx_text(line, 160, 37, COL_LILAC_D, ALIGN_CENTER);
    int y = 44;
    gfx_text("Gioco", 16, y, COL_HOT, ALIGN_LEFT);
    gfx_text("Liv.", 150, y, COL_HOT, ALIGN_RIGHT);
    gfx_text("Round", 190, y, COL_HOT, ALIGN_RIGHT);
    gfx_text("Domande", 244, y, COL_HOT, ALIGN_RIGHT);
    gfx_text("Giuste 1°", 304, y, COL_HOT, ALIGN_RIGHT);
    for (int g = 0; g < GAME_COUNT; g++) {
        y = 53 + g * 9;
        gfx_text(GAME_TITLES[g], 16, y, COL_INK, ALIGN_LEFT);
        snprintf(line, sizeof(line), "%d", p->level[g]);
        gfx_text(line, 150, y, COL_INK, ALIGN_RIGHT);
        snprintf(line, sizeof(line), "%d", p->rounds[g]);
        gfx_text(line, 190, y, COL_INK, ALIGN_RIGHT);
        snprintf(line, sizeof(line), "%d", p->answers[g]);
        gfx_text(line, 244, y, COL_INK, ALIGN_RIGHT);
        if (p->answers[g] > 0)
            snprintf(line, sizeof(line), "%d%%", p->first_try[g] * 100 / p->answers[g]);
        else
            snprintf(line, sizeof(line), "-");
        gfx_text(line, 304, y, COL_INK, ALIGN_RIGHT);
    }
    long min = (p->play_seconds + G.play_frames / FPS) / 60;
    snprintf(line, sizeof(line), "Sessioni %d  Round %d  Stelle %d  Tempo %ldh%02ld", p->sessions,
             p->rounds_total, p->stars_total, min / 60, min % 60);
    gfx_text(line, 160, 198, COL_INK, ALIGN_CENTER);
    int n = 0, ns = 0;
    for (int i = 0; i < ITEM_COUNT; i++)
        n += (p->owned >> i) & 1u;
    for (int i = 0; i < STICKER_COUNT; i++)
        ns += (p->stickers >> i) & 1u;
    snprintf(line, sizeof(line), "Storia %d: luoghi %d/%d  Trucchi %d/%d  Adesivi %d/%d", p->arc + 1,
             p->chapter < CH_COUNT ? p->chapter : CH_COUNT, CH_COUNT, n, ITEM_COUNT, ns, STICKER_COUNT);
    gfx_text(line, 160, 207, COL_INK, ALIGN_CENTER);
    if (G.slot == 1)
        snprintf(line, sizeof(line), "B: indietro   -   tutte le risposte: deva_adventures_log.csv");
    else
        snprintf(line, sizeof(line), "B: indietro   -   tutte le risposte: deva_adventures_%d_log.csv", G.slot);
    ui_footer(line);
}

static void draw_crediti(void)
{
    const sprite_t *logo = gfx_sprite("logo");
    gfx_blit(logo, (SCREEN_W - logo->w) / 2, 6, 0);
    ui_panel(18, 84, 284, 134);
    static const char *LINES[] = {
        ("Versione " GAME_VERSION),
        "Un gioco educativo fatto su misura per Deva:",
        "numeri, monete, misure, lettere, forme, coding e quattro storie.",
        "",
        "Disegni, musiche, storie e codice: originali.",
        "Voce: sintesi vocale offline Piper (it_IT paola).",
        "Librerie: stb_image, stb_vorbis (dominio pubblico),",
        "interfaccia libretro (MIT).",
        "",
        "Codice del gioco: licenza MIT.",
    };
    for (int i = 0; i < ARRAY_LEN(LINES); i++)
        gfx_text(LINES[i], 160, 92 + i * 12, i == 0 ? COL_HOT : COL_INK, ALIGN_CENTER);
    ui_footer("B: indietro");
}

static void draw(void)
{
    ui_backdrop("bg_palco", 170);
    switch (O.page) {
    case PG_MAIN: draw_main(); break;
    case PG_LIVELLI: draw_livelli(); break;
    case PG_PROGRESSI: draw_progressi(); break;
    default: draw_crediti(); break;
    }
}

const scene_t SCENE_OPZIONI = {enter, update, draw};
