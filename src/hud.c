/* Deva's Awesome Adventures - console drawings for the tutorial and the dance.
 * SPDX-License-Identifier: MIT
 */
#include "hud.h"

#include "audio.h"
#include "gfx.h"

void hud_console_zoom(int x, int y, int highlight, bool lit, int k)
{
    static const char *NAMES[] = {"console", "console_dpad", "console_a", "console_b"};
    int h = (highlight >= HUD_DPAD && highlight <= HUD_B && lit) ? highlight : HUD_NONE;
    gfx_blit_scaled(gfx_sprite(NAMES[h]), x, y, k, 0);
}

void hud_console(int x, int y, int highlight, bool lit) { hud_console_zoom(x, y, highlight, lit, 1); }

void hud_dpad(int x, int y, int dir)
{
    static const char *NAMES[] = {"dpad_up", "dpad_down", "dpad_left", "dpad_right"};
    gfx_blit(gfx_sprite(dir >= DIR_UP && dir <= DIR_RIGHT ? NAMES[dir] : "dpad"), x, y, 0);
}

int hud_tutorial_highlight(void)
{
    sound_t cur = voice_current();
    if (cur < 0)
        return HUD_NONE;
    if (cur == snd_find(SND_VOICE, "istruzioni")) {
        /* "Usa la croce nera per scegliere, | poi premi il bottone rosso!":
           the comma pause falls at about 52% of the clip */
        int len_ms = snd_length_frames(cur) * 1000 / FPS;
        return voice_pos_ms() < len_ms * 52 / 100 ? HUD_DPAD : HUD_A;
    }
    if (cur == snd_find(SND_VOICE, "aiuto_b"))
        return HUD_B;
    return HUD_NONE;
}
