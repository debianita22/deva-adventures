/* Deva's Awesome Adventures - the letter keyboard: she writes her name (or a
 * grown-up does) for a new profile, or to change one. Each letter says its
 * name when highlighted, like in "Il mio nome". The arrow deletes, the tick
 * confirms; B goes back without changes.
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "ui.h"

#define COLS 7
#define KEYS 28 /* A..Z, delete, done */
#define K_DEL 26
#define K_OK 27
#define KW 36
#define KH 34
#define KX ((SCREEN_W - COLS * KW) / 2)
#define KY 78

static struct {
    char name[NAME_MAX_LEN + 1];
    int len, sel, t, done_t;
} K;

static void say_key(int k)
{
    if (k < 26) {
        char id[8];
        snprintf(id, sizeof(id), "lt_%c", 'a' + k);
        say(id);
    } else {
        voice_stop();
    }
}

static void select_key(int k)
{
    K.sel = clampi(k, 0, KEYS - 1);
    BOT("tastiera sel=%d key=%c\n", K.sel, K.sel < 26 ? 'A' + K.sel : (K.sel == K_DEL ? '<' : '!'));
}

static void enter(void)
{
    memset(&K, 0, sizeof(K));
    if (!G.edit_new) {
        progress_t p;
        const char *cur = NULL;
        if (G.edit_slot == G.slot)
            cur = child_name();
        else if (progress_load(&p, dir_save(), G.edit_slot, 1))
            cur = p.name;
        if (cur) {
            snprintf(K.name, sizeof(K.name), "%.10s", cur);
            K.len = (int)strlen(K.name);
        }
    }
    music_play(snd_find(SND_MUSIC, "palco"));
    select_key(0);
    say("tas_intro");
}

static void update(void)
{
    K.t++;
    if (K.done_t > 0) { /* "che bel nome!", then back to the profiles */
        if (++K.done_t > 50 && (!voice_busy() || K.done_t > 200))
            game_goto(SC_SALVATAGGI);
        return;
    }
    int dx = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    int dy = btn_pressed(BTN_UP) ? -1 : (btn_pressed(BTN_DOWN) ? 1 : 0);
    if (dx || dy) {
        int col = (K.sel % COLS + dx + COLS) % COLS, row = (K.sel / COLS + dy + KEYS / COLS) % (KEYS / COLS);
        select_key(row * COLS + col);
        sfx("blip");
        say_key(K.sel);
    } else if (btn_pressed(BTN_A)) {
        if (K.sel < 26) {
            if (K.len < NAME_MAX_LEN) {
                K.name[K.len++] = (char)('A' + K.sel);
                K.name[K.len] = 0;
                sfx("pop");
                say_key(K.sel);
                BOT("tastiera name=%s\n", K.name);
            } else {
                sfx("boop");
            }
        } else if (K.sel == K_DEL) {
            if (K.len > 0) {
                K.name[--K.len] = 0;
                sfx("whoosh");
                BOT("tastiera name=%s\n", K.name);
            } else {
                sfx("boop");
            }
        } else if (K.len >= 1) {
            if (G.edit_new)
                game_new_profile(G.edit_slot, K.name);
            else
                game_rename_profile(G.edit_slot, K.name);
            sfx("fanfare");
            fx_sparkles(160, 48, 24, 16);
            say("tas_fatto");
            K.done_t = 1;
            BOT("tastiera done name=%s\n", K.name);
        } else {
            sfx("boop");
            say("tas_intro");
        }
    } else if (btn_pressed(BTN_B)) {
        sfx("blip");
        game_goto(SC_SALVATAGGI);
    }
}

static void draw(void)
{
    ui_backdrop("bg_palco", 120);
    ui_title(G.edit_new ? "COME TI CHIAMI?" : "CAMBIA NOME", 16);
    /* the name so far, with a blinking cursor */
    ui_panel(50, 32, 220, 32);
    int w = gfx_text_big_width(K.name, 1), x0 = 160 - w / 2;
    if (K.len > 0)
        gfx_text_big(K.name, 160, 48, NUM_SELECTED, 1);
    if (K.done_t == 0 && K.len < NAME_MAX_LEN && ((K.t / 20) & 1))
        gfx_fill_rect(K.len > 0 ? x0 + w + 3 : 156, 56, 8, 3, COL_HOT);
    char ch[2] = {0, 0};
    for (int k = 0; k < KEYS; k++) {
        int x = KX + (k % COLS) * KW, y = KY + (k / COLS) * KH;
        bool sel = k == K.sel;
        const sprite_t *card = gfx_sprite(sel ? "scard_sel" : "scard");
        int cx = x + KW / 2, cy = y + KH / 2;
        gfx_blit(card, cx - card->w / 2, cy - card->h / 2, 0);
        if (sel)
            ui_glow(cx - 18, cy - 18, 35, 35);
        if (k < 26) {
            ch[0] = (char)('A' + k);
            gfx_text_big(ch, cx, cy, sel ? NUM_SELECTED : NUM_NORMAL, 1);
        } else {
            const sprite_t *s = gfx_sprite(k == K_DEL ? "arrow_left" : "mark_ok");
            gfx_blit(s, cx - s->w / 2, cy - s->h / 2, 0);
        }
    }
    ui_footer("Croce: scegli   A: scrivi   freccia: cancella   B: indietro");
    fx_draw();
}

const scene_t SCENE_TASTIERA = {enter, update, draw};
