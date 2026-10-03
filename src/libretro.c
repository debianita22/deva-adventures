/* Deva's Awesome Adventures - libretro entry points.
 * SPDX-License-Identifier: MIT
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "libretro.h"

#include "audio.h"
#include "common.h"
#include "game.h"
#include "gfx.h"

#ifndef GAME_DATA_DIR
#define GAME_DATA_DIR "/usr/share/deva_adventures"
#endif

static retro_environment_t env_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;
static struct retro_rumble_interface s_rumble;
static bool s_have_rumble, s_can_dupe, s_bitmasks, s_loaded;
static int s_rumble_frames;
static char s_data_dir[512], s_save_dir[512];
static int16_t s_audio[SAMPLES_PER_FRAME * 2];
static uint32_t s_rng = 0x9e3779b9u;

/* ------------------------------------------------------------------ services */

void log_msg(log_level_t level, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    static const enum retro_log_level MAP[] = {RETRO_LOG_DEBUG, RETRO_LOG_INFO, RETRO_LOG_WARN, RETRO_LOG_ERROR};
    if (log_cb)
        log_cb(MAP[level], "[deva] %s", buf);
    else if (level >= LOG_INFO)
        fprintf(stderr, "[deva] %s", buf);
}

void rng_seed(uint32_t seed) { s_rng = seed ? seed : 0x9e3779b9u; }

uint32_t rng_next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

int rng_range(int lo, int hi) { return hi <= lo ? lo : lo + (int)(rng_next() % (uint32_t)(hi - lo + 1)); }

const char *dir_data(void) { return s_data_dir; }
const char *dir_save(void) { return s_save_dir; }

void path_join(char *out, size_t n, const char *dir, const char *name) { snprintf(out, n, "%s/%s", dir, name); }

void frontend_rumble(int strength, int frames)
{
    if (!s_have_rumble || !s_rumble.set_rumble_state)
        return;
    s_rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, (uint16_t)(strength ? 0x9000 : 0));
    s_rumble_frames = frames;
}

void frontend_shutdown(void)
{
    if (env_cb)
        env_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

static bool has_assets(const char *dir)
{
    char p[600];
    struct stat st;
    snprintf(p, sizeof(p), "%s/gfx/atlas.txt", dir);
    return stat(p, &st) == 0;
}

/* ------------------------------------------------------------------ libretro API */

RETRO_API unsigned retro_api_version(void) { return RETRO_API_VERSION; }

RETRO_API void retro_set_environment(retro_environment_t cb)
{
    env_cb = cb;
    bool no_game = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

RETRO_API void retro_init(void)
{
    struct retro_log_callback logging;
    if (env_cb && env_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    const char *seed = getenv("DEVA_SEED"); /* reproducible runs for tests */
    rng_seed(seed ? (uint32_t)strtoul(seed, NULL, 10) : (uint32_t)time(NULL) * 2654435761u);
}

RETRO_API void retro_deinit(void) {}

RETRO_API void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = "Deva's Awesome Adventures";
    info->library_version = GAME_VERSION;
    info->valid_extensions = "";
    info->need_fullpath = false;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info)
{
    memset(info, 0, sizeof(*info));
    info->geometry.base_width = SCREEN_W;
    info->geometry.base_height = SCREEN_H;
    info->geometry.max_width = SCREEN_W;
    info->geometry.max_height = SCREEN_H;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = FPS;
    info->timing.sample_rate = AUDIO_RATE;
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

RETRO_API void retro_reset(void)
{
    if (s_loaded)
        game_goto(SC_TITLE);
}

static uint32_t read_buttons(void)
{
    static const struct {
        unsigned id;
        int btn;
    } MAP[] = {
        {RETRO_DEVICE_ID_JOYPAD_UP, BTN_UP},       {RETRO_DEVICE_ID_JOYPAD_DOWN, BTN_DOWN},
        {RETRO_DEVICE_ID_JOYPAD_LEFT, BTN_LEFT},   {RETRO_DEVICE_ID_JOYPAD_RIGHT, BTN_RIGHT},
        {RETRO_DEVICE_ID_JOYPAD_A, BTN_A},         {RETRO_DEVICE_ID_JOYPAD_B, BTN_B},
        {RETRO_DEVICE_ID_JOYPAD_X, BTN_X},         {RETRO_DEVICE_ID_JOYPAD_Y, BTN_Y},
        {RETRO_DEVICE_ID_JOYPAD_START, BTN_START}, {RETRO_DEVICE_ID_JOYPAD_SELECT, BTN_SELECT},
        {RETRO_DEVICE_ID_JOYPAD_L, BTN_L},         {RETRO_DEVICE_ID_JOYPAD_R, BTN_R},
    };
    uint32_t mask = 0;
    if (s_bitmasks) {
        int16_t bits = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
        for (int i = 0; i < ARRAY_LEN(MAP); i++)
            if (bits & (1 << MAP[i].id))
                mask |= 1u << MAP[i].btn;
    } else {
        for (int i = 0; i < ARRAY_LEN(MAP); i++)
            if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, MAP[i].id))
                mask |= 1u << MAP[i].btn;
    }
    return mask; /* analog sticks deliberately ignored */
}

/* the last picture sent, to tell the frontend when nothing changed (1.0: compared, not hashed - a
   memcmp stops at the first different pixel and never mistakes two pictures for one) */
static uint16_t s_last_fb[SCREEN_W * SCREEN_H];
static bool s_last_valid;

RETRO_API void retro_run(void)
{
    input_poll_cb();
    input_set(read_buttons());
    game_frame();

    if (s_rumble_frames > 0 && --s_rumble_frames == 0)
        frontend_rumble(0, 0);

    /* identical frame: let the frontend reuse the previous one */
    bool same = s_last_valid && !memcmp(g_fb, s_last_fb, sizeof(s_last_fb));
    if (s_can_dupe && same)
        video_cb(NULL, SCREEN_W, SCREEN_H, SCREEN_W * sizeof(uint16_t));
    else
        video_cb(g_fb, SCREEN_W, SCREEN_H, SCREEN_W * sizeof(uint16_t));
    if (!same) {
        memcpy(s_last_fb, g_fb, sizeof(s_last_fb));
        s_last_valid = true;
    }

    audio_render(s_audio, SAMPLES_PER_FRAME);
    audio_batch_cb(s_audio, SAMPLES_PER_FRAME);
}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
    (void)game;
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_msg(LOG_ERROR, "RGB565 not supported by the frontend\n");
        return false;
    }
    const char *sys = NULL, *sav = NULL;
    if (!env_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &sys) || !sys)
        sys = ".";
    if (!env_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &sav) || !sav || !*sav)
        sav = sys;
    /* data: <system>/deva_adventures wins (parents can customise it), else the packaged copy */
    snprintf(s_data_dir, sizeof(s_data_dir), "%s/deva_adventures", sys);
    if (!has_assets(s_data_dir) && has_assets(GAME_DATA_DIR))
        snprintf(s_data_dir, sizeof(s_data_dir), "%s", GAME_DATA_DIR);
    snprintf(s_save_dir, sizeof(s_save_dir), "%s", sav);
    log_msg(LOG_INFO, "data %s, saves %s\n", s_data_dir, s_save_dir);

    bool dupe = false;
    s_can_dupe = env_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE, &dupe) && dupe;
    s_bitmasks = env_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
    s_have_rumble = env_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &s_rumble);

    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Scegli a sinistra / Passo a sinistra"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Scegli a destra / Passo a destra"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Braccia in alto / Su"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Giu'"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Conferma (bottone rosso)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Ripeti la domanda / Indietro (bottone giallo)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Salvataggi: cambia nome"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Ripeti la domanda / Salvataggi: cancella"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pausa (tieni premuto)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Pausa (tieni premuto)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Per i grandi: L + R tenuti 2 s (opzioni, salvataggi, esci)"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Per i grandi: L + R tenuti 2 s (opzioni, salvataggi, esci)"},
        {0, 0, 0, 0, NULL},
    };
    env_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    if (!gfx_init(s_data_dir)) {
        log_msg(LOG_ERROR, "graphics missing: copy the 'deva_adventures' folder into %s\n", sys);
        return false;
    }
    if (!audio_init(s_data_dir))
        log_msg(LOG_WARN, "no audio found in %s: the game will be silent\n", s_data_dir);
    game_init();
    s_loaded = true;
    return true;
}

RETRO_API bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
    (void)type;
    (void)info;
    (void)num;
    return false;
}

RETRO_API void retro_unload_game(void)
{
    if (!s_loaded)
        return;
    game_shutdown();
    audio_free();
    gfx_free();
    s_loaded = false;
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
RETRO_API size_t retro_serialize_size(void) { return 0; }
RETRO_API bool retro_serialize(void *data, size_t size)
{
    (void)data;
    (void)size;
    return false;
}
RETRO_API bool retro_unserialize(const void *data, size_t size)
{
    (void)data;
    (void)size;
    return false;
}
RETRO_API void *retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}
RETRO_API size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0;
}
RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}
