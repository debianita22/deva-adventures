/* Deva's Awesome Adventures - saves: three profiles side by side, each with its
 * little Deva dressed as that child left her, the name in big letters, the gems
 * and the stars. A plays with the highlighted profile. The rest is for the
 * grown-ups: a new profile on an empty card (A held for two seconds, then the
 * letter keyboard), a new name (X held for two seconds) and deleting (Y, then X
 * and Y held together; a .bak copy of the files is kept). A quick press only
 * says "questo è per mamma e papà".
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>

#include "anim.h"
#include "audio.h"
#include "fx.h"
#include "game.h"
#include "gfx.h"
#include "story.h"
#include "ui.h"

#define CW 96
#define CH 150
#define CY 40

static struct {
    bool filled[PROFILE_COUNT];
    progress_t prof[PROFILE_COUNT];
    hero_t hero[PROFILE_COUNT];
    int sel, t;
    bool ask;   /* the delete question is open */
    int hold;   /* frames X+Y have been held on it */
    int press;  /* frames A (new profile) or X (new name) have been held */
    int press_btn;
} S;

static int card_x(int i) { return 8 + i * (CW + 8); }

static void load_all(void)
{
    for (int i = 0; i < PROFILE_COUNT; i++) {
        int slot = i + 1;
        if (slot == G.slot) {
            S.filled[i] = true;
            S.prof[i] = G.prog;
            continue;
        }
        S.filled[i] = profile_exists(dir_save(), slot) &&
                      progress_load(&S.prof[i], dir_save(), slot, G.cfg.start_level);
        if (S.filled[i] && !S.prof[i].name[0])
            snprintf(S.prof[i].name, sizeof(S.prof[i].name), "%s", slot == 1 ? G.cfg.child_name : "?");
    }
}

static void select_card(int i)
{
    S.sel = clampi(i, 0, PROFILE_COUNT - 1);
    BOT("salvataggi sel=%d filled=%d active=%d\n", S.sel, S.filled[S.sel], S.sel + 1 == G.slot);
}

static void enter(void)
{
    S.t = S.hold = S.press = 0;
    S.ask = false;
    load_all();
    for (int i = 0; i < PROFILE_COUNT; i++) {
        hero_init(&S.hero[i], card_x(i) + CW / 2, CY + 88);
        S.hero[i].blink = 40 + i * 37;
    }
    music_play(snd_find(SND_MUSIC, "palco"));
    select_card(G.slot - 1);
    say("sal_intro");
}

static void open_editor(bool is_new)
{
    G.edit_slot = S.sel + 1;
    G.edit_new = is_new;
    sfx("blip");
    game_goto(SC_TASTIERA);
}

/* a door for the grown-ups: the keyboard opens only after two seconds */
static void update_press(void)
{
    if (btn_held(S.press_btn)) {
        if (++S.press >= HOLD_FRAMES) {
            bool is_new = !S.filled[S.sel];
            S.press = 0;
            sfx("sparkle");
            BOT("salvataggi hold done new=%d\n", is_new);
            open_editor(is_new);
        } else if (S.press % 10 == 0) {
            sfx("blip");
        }
    } else {
        S.press = 0;
        say("m_grandi");
        BOT("salvataggi hold released\n");
    }
}

static void start_press(int btn)
{
    S.press = 1;
    S.press_btn = btn;
}

static void update(void)
{
    S.t++;
    for (int i = 0; i < PROFILE_COUNT; i++) {
        S.hero[i].talking = false;
        hero_update(&S.hero[i]);
    }
    if (S.press > 0) {
        update_press();
        return;
    }
    if (S.ask) { /* deleting: X and Y held together (not something a child does by chance) */
        if (btn_held(BTN_X) && btn_held(BTN_Y)) {
            if (++S.hold >= HOLD_FRAMES) {
                game_delete_profile(S.sel + 1);
                sfx("whoosh");
                S.ask = false;
                S.hold = 0;
                load_all();
                select_card(S.sel);
            }
        } else {
            S.hold = 0;
            if (btn_pressed(BTN_B) || btn_pressed(BTN_A)) {
                S.ask = false;
                sfx("blip");
            }
        }
        return;
    }
    int dir = btn_pressed(BTN_LEFT) ? -1 : (btn_pressed(BTN_RIGHT) ? 1 : 0);
    if (dir && S.sel + dir >= 0 && S.sel + dir < PROFILE_COUNT) {
        select_card(S.sel + dir);
        sfx("blip");
        if (!S.filled[S.sel])
            say("sal_nuovo");
        else
            voice_stop();
    } else if (btn_pressed(BTN_A)) {
        if (!S.filled[S.sel]) {
            start_press(BTN_A);
        } else {
            sfx("star");
            if (S.sel + 1 != G.slot)
                game_use_profile(S.sel + 1);
            G.title_sel = 0;
            game_goto(SC_TITLE);
        }
    } else if (btn_pressed(BTN_X) && S.filled[S.sel]) {
        start_press(BTN_X);
    } else if (btn_pressed(BTN_Y) && S.filled[S.sel]) {
        S.ask = true;
        S.hold = 0;
        sfx("boop");
        BOT("salvataggi ask slot=%d\n", S.sel + 1);
    } else if (btn_pressed(BTN_B) || btn_pressed(BTN_START)) {
        sfx("blip");
        game_goto(SC_TITLE);
    }
}

static void draw_card(int i)
{
    int x = card_x(i), y = CY;
    bool sel = i == S.sel;
    ui_card(x, y, CW, CH, sel);
    if (sel && S.press > 0)
        ui_hold_stars(x - 3, y - 3, CW + 6, CH + 6, S.press, HOLD_FRAMES);
    if (i + 1 == G.slot) /* in use: a crown on top */
        gfx_blit(gfx_sprite("mark_corona"), x + CW / 2 - 9, y - 9, 0);
    char line[48];
    snprintf(line, sizeof(line), "%d", i + 1);
    gfx_text(line, x + 6, y + 5, COL_LILAC_D, ALIGN_LEFT);
    if (!S.filled[i]) {
        const sprite_t *plus = gfx_sprite("plus");
        int bob = sel ? -swing((int)G.frame, 48, 2) : 0;
        gfx_blit_scaled(plus, x + CW / 2 - 24, y + 40 + bob, 3, 0);
        gfx_text("Nuovo", x + CW / 2, y + 100, COL_INK, ALIGN_CENTER);
        return;
    }
    const progress_t *p = &S.prof[i];
    hero_draw(&S.hero[i], p);
    const char *name = p->name;
    int nw = gfx_text_big_width(name, 1);
    if (nw <= CW - 6)
        gfx_text_big(name, x + CW / 2, y + 102, sel ? NUM_SELECTED : NUM_NORMAL, 1);
    else
        gfx_text(name, x + CW / 2, y + 98, COL_INK, ALIGN_CENTER);
    /* gems (the stars of the second adventure) and stars won */
    static const char *const GEM[ARC_COUNT] = {"gemma_verde", "stella_oro", "nota_oro", "chiave_oro"};
    const sprite_t *gem = gfx_sprite(GEM[clampi(p->arc, 0, ARC_COUNT - 1)]), *star = gfx_sprite("lstar_on");
    gfx_blit(gem, x + 8, y + 116 + (18 - gem->h) / 2, 0);
    snprintf(line, sizeof(line), "%d/5", p->chapter < 5 ? p->chapter : 5);
    gfx_text(line, x + 30, y + 121, COL_INK, ALIGN_LEFT);
    gfx_blit(star, x + 54, y + 120, 0);
    snprintf(line, sizeof(line), "%d", p->stars_total);
    gfx_text(line, x + 67, y + 121, COL_INK, ALIGN_LEFT);
    if (p->last_day[0]) { /* "2026-09-28" -> "28/09/2026" */
        snprintf(line, sizeof(line), "%.2s/%.2s/%.4s", p->last_day + 8, p->last_day + 5, p->last_day);
        gfx_text(line, x + CW / 2, y + 136, COL_LILAC_D, ALIGN_CENTER);
    }
}

static void draw(void)
{
    ui_backdrop("bg_palco", 120);
    ui_title("SALVATAGGI", 18);
    for (int i = 0; i < PROFILE_COUNT; i++)
        draw_card(i);
    if (S.filled[S.sel])
        ui_footer("A: gioca  X (tieni): nome  Y: cancella  B: indietro");
    else
        ui_footer("A (tieni premuto): nuovo profilo   B: indietro");
    if (S.ask) {
        gfx_fade(COL_DEEP, 120);
        ui_panel(30, 62, 260, 108);
        const char *name = S.prof[S.sel].name;
        char line[80];
        bool active = S.sel + 1 == G.slot;
        if (active)
            snprintf(line, sizeof(line), "Ricominciare da capo con %s?", name);
        else
            snprintf(line, sizeof(line), "Cancellare il profilo %d (%s)?", S.sel + 1, name);
        gfx_text(line, 160, 74, COL_INK, ALIGN_CENTER);
        gfx_text("Livelli, trucchi, stelle e storia", 160, 90, COL_INK, ALIGN_CENTER);
        gfx_text("andranno persi (resta una copia .bak).", 160, 101, COL_INK, ALIGN_CENTER);
        gfx_text("Tieni premuti insieme X e Y per confermare", 160, 122, COL_HOT, ALIGN_CENTER);
        gfx_text("A o B: annulla", 160, 134, COL_LILAC_D, ALIGN_CENTER);
        ui_bar(60, 152, 200, S.hold, HOLD_FRAMES);
    }
    fx_draw();
}

const scene_t SCENE_SALVATAGGI = {enter, update, draw};
