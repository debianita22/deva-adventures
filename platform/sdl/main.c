/* Deva's Awesome Adventures - the game on its own, for a PC without RetroArch: Linux (1.1), Windows
 * and macOS (1.2).
 *
 * The very same core (its sources are linked in) in a window of its own. SDL2 gives the picture, the
 * sound and the gamepads, and is loaded at run time, so the program builds with a bare C compiler:
 * libSDL2-2.0.so.0 of the system on Linux (glibc >= 2.17, SDL2 or sdl2-compat), SDL2.dll next to the
 * program on Windows, the SDL2.framework inside the app on macOS. What differs between the three is
 * in os.c.
 *
 *   deva-adventures [--fullscreen | --window] [--scale N] [--data DIR] [--saves DIR] [--no-audio]
 *                   [--no-pause] [--pad colori|posizione] [--verbose] [--check] [--version] [--help]
 *
 * Data: next to the program (the unpacked package; the app's Resources on macOS), else
 * <program>/../share, /usr/local/share, /usr/share, the DATA_DIR of the build.
 * Saves: ~/.local/share/deva-adventures ($XDG_DATA_HOME), ~/Library/Application Support/deva-adventures,
 * %APPDATA%\deva-adventures; one copy of the game at a time (a lock there). The window freezes the
 * game while it is not in front: the time does not run. Messages are in Italian, for the grown-ups.
 * DEVA_TEST_FRAMES=N ends the game in order after N frames (the smoke tests of the CI).
 */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "common.h"
#include "libretro.h"
#include "os.h"
#include "plat.h"
#include "sdl2_api.h"

#define PATHLEN 1024

/* stb_image is compiled into the core (third_party/third_party_impl.c): the window icon */
unsigned char *stbi_load(const char *filename, int *x, int *y, int *comp, int req_comp);
void stbi_image_free(void *retval_from_stbi_load);

#define APP "deva-adventures"
#define TITLE "Deva's Awesome Adventures"
#define MAX_PADS 4

/* ------------------------------------------------------------------ SDL2, loaded at run time */
static struct {
#define X(ret, name, args) ret(*name) args;
    SDL2_FUNCTIONS(X)
    SDL2_OPTIONAL_FUNCTIONS(X)
#undef X
} S;

static char s_exe_dir[PATHLEN], s_sys_dir[PATHLEN], s_save_dir[PATHLEN];

/* a path from a format; false when it would not fit (never a path cut short) */
static bool fit(char *out, size_t n, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 3, 4)))
#endif
    ;
static bool fit(char *out, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int k = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return k >= 0 && (size_t)k < n;
}

static bool load_sdl(char *why, size_t n)
{
    char cand[6][PATHLEN];
    int count = 0;
    const char *forced = getenv("DEVA_SDL2_LIB"); /* another name for the library, or a test */
    if (forced && *forced) {
        count += fit(cand[count], PATHLEN, "%s", forced);
    } else {
#if defined(_WIN32)
        count += fit(cand[count], PATHLEN, "%s\\SDL2.dll", s_exe_dir); /* (that one, not another found elsewhere) */
#elif defined(__APPLE__)
        count += fit(cand[count], PATHLEN, "%s/../Frameworks/SDL2.framework/SDL2", s_exe_dir); /* the app's */
        count += fit(cand[count], PATHLEN, "%s/libSDL2-2.0.0.dylib", s_exe_dir);
        count += fit(cand[count], PATHLEN, "libSDL2-2.0.0.dylib"); /* (/usr/local/lib: Homebrew on Intel) */
        count += fit(cand[count], PATHLEN, "/opt/homebrew/lib/libSDL2-2.0.0.dylib");
        count += fit(cand[count], PATHLEN, "SDL2.framework/SDL2"); /* (/Library/Frameworks) */
#else
        count += fit(cand[count], PATHLEN, "libSDL2-2.0.so.0");
        count += fit(cand[count], PATHLEN, "libSDL2.so");
#endif
    }
    void *lib = NULL;
    for (int i = 0; i < count && !lib; i++)
        lib = os_lib_open(cand[i]);
    if (!lib) {
        os_lib_error(why, n);
        return false;
    }
#define X(ret, name, args)                                                                                    \
    *(void **)&S.name = os_lib_sym(lib, #name);                                                               \
    if (!S.name) {                                                                                            \
        snprintf(why, n, "SDL2 troppo vecchio: manca %s (serve la 2.0.9 o successiva)", #name);               \
        S.SDL_ShowSimpleMessageBox = NULL;                                                                    \
        return false;                                                                                         \
    }
    SDL2_FUNCTIONS(X)
#undef X
#define X(ret, name, args) *(void **)&S.name = os_lib_sym(lib, #name);
    SDL2_OPTIONAL_FUNCTIONS(X)
#undef X
    return true;
}

/* ------------------------------------------------------------------ messages for the grown-ups */
static SDL_Window *s_window;
static FILE *s_log; /* deva-adventures.log in the saves folder (the run before: .log.1) */
static bool s_verbose, s_no_dialog;

static void tell_user(bool error, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
static void tell_user(bool error, const char *fmt, ...)
{
    char msg[1200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s: %s\n", APP, msg);
    if (s_log) {
        fprintf(s_log, "[app] %s\n", msg);
        fflush(s_log);
    }
    /* a window only when started from the desktop: from a terminal the line above is enough (and
       DEVA_NO_DIALOG keeps the automated tests from waiting on a dialog nobody will close) */
    const char *quiet = getenv("DEVA_NO_DIALOG");
    if (s_no_dialog || os_is_terminal() || (quiet && *quiet))
        return;
    if (S.SDL_ShowSimpleMessageBox &&
        S.SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : SDL_MESSAGEBOX_WARNING, TITLE, msg, s_window) == 0)
        return;
    os_dialog(error, TITLE, msg); /* no SDL: the system's own */
}

static void note(const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;
static void note(const char *fmt, ...) /* only the log (and the terminal with --verbose) */
{
    va_list ap;
    if (s_log) {
        va_start(ap, fmt);
        fputs("[app] ", s_log);
        vfprintf(s_log, fmt, ap);
        fputc('\n', s_log);
        fflush(s_log);
        va_end(ap);
    }
    if (s_verbose) {
        va_start(ap, fmt);
        fprintf(stderr, "%s: ", APP);
        vfprintf(stderr, fmt, ap);
        fputc('\n', stderr);
        va_end(ap);
    }
}

/* ------------------------------------------------------------------ folders */
static void find_exe_dir(void)
{
    if (!os_exe_dir(s_exe_dir, sizeof(s_exe_dir)))
        fit(s_exe_dir, sizeof(s_exe_dir), ".");
}

static bool is_sep(char c)
{
#if defined(_WIN32)
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

static bool has_data(const char *dir)
{
    char p[PATHLEN];
    struct stat st;
    return fit(p, sizeof(p), "%s/gfx/atlas.txt", dir) && stat(p, &st) == 0;
}

/* The core wants the folder that holds deva_adventures/ (RetroArch's "system" folder). DIR given by
   --data or DEVA_ADVENTURES_DATA is that folder or deva_adventures/ itself. */
static bool data_from(const char *given)
{
    char dir[PATHLEN], p[PATHLEN];
    if (!fit(dir, sizeof(dir), "%s", given))
        return false;
    for (size_t n = strlen(dir); n > 1 && is_sep(dir[n - 1]); n--) /* (a trailing slash) */
        dir[n - 1] = 0;
    const char *base = dir;
    for (const char *c = dir; *c; c++)
        if (is_sep(*c))
            base = c + 1;
    if (!strcmp(base, "deva_adventures") && has_data(dir)) {
        if (base == dir)
            return fit(s_sys_dir, sizeof(s_sys_dir), ".");
        if (base == dir + 1)
            return fit(s_sys_dir, sizeof(s_sys_dir), "/");
        return fit(s_sys_dir, sizeof(s_sys_dir), "%.*s", (int)(base - dir - 1), dir);
    }
    if (fit(p, sizeof(p), "%s/deva_adventures", dir) && has_data(p))
        return fit(s_sys_dir, sizeof(s_sys_dir), "%s", dir);
    return false;
}

static bool find_data(const char *arg)
{
    if (arg) {
        if (data_from(arg))
            return true;
        tell_user(true, "Non trovo i dati del gioco in %s: serve la cartella deva_adventures (con gfx, voce, sfx "
                   "e musica) o la cartella che la contiene.", arg);
        return false;
    }
    const char *env = getenv("DEVA_ADVENTURES_DATA");
    if (env && *env && data_from(env))
        return true;
    char p[PATHLEN];
#if defined(__APPLE__)
    if (fit(p, sizeof(p), "%s/../Resources", s_exe_dir) && data_from(p)) /* inside the app */
        return true;
#endif
    if (data_from(s_exe_dir))
        return true;
    if (fit(p, sizeof(p), "%s/../share", s_exe_dir) && data_from(p))
        return true;
#if !defined(_WIN32)
    if (data_from("/usr/local/share") || data_from("/usr/share"))
        return true;
#if defined(__APPLE__)
    if (data_from("/opt/homebrew/share"))
        return true;
#endif
#ifdef GAME_DATA_DIR
    if (data_from(GAME_DATA_DIR)) /* (make install-linux DATA_DIR=...) */
        return true;
#endif
#endif
    tell_user(true, "Non trovo i dati del gioco: la cartella deva_adventures deve stare accanto al programma (%s) "
               "oppure va indicata con --data CARTELLA.", s_exe_dir);
    return false;
}

static bool find_saves(const char *arg)
{
    bool ok;
    if (arg)
        ok = fit(s_save_dir, sizeof(s_save_dir), "%s", arg);
    else
        ok = os_saves_dir(s_save_dir, sizeof(s_save_dir), APP) ||
             fit(s_save_dir, sizeof(s_save_dir), "%s" OS_SEP "salvataggi", s_exe_dir);
    errno = ENAMETOOLONG;
    if (!ok || !os_make_dirs(s_save_dir) || !os_writable(s_save_dir)) {
        tell_user(true, "Non posso scrivere i salvataggi in %s (%s).", s_save_dir, strerror(errno));
        return false;
    }
    return true;
}

/* one copy of the game at a time: two would write the same saves */
static bool take_lock(void)
{
    char p[PATHLEN];
    if (!fit(p, sizeof(p), "%s/.deva-adventures.lock", s_save_dir))
        return true;
    return os_lock(p) != 0; /* (a folder that takes no lock: go on) */
}

/* ------------------------------------------------------------------ the core's callbacks */
static SDL_Renderer *s_renderer;
static SDL_Texture *s_texture;
static SDL_AudioDeviceID s_audio;
static SDL_GameController *s_pads[MAX_PADS];
static bool s_quit, s_new_frame;
static uint32_t s_latched, s_bits; /* presses seen between two frames, the buttons of this frame */

static const struct {
    int key;
    unsigned id;
} KEYS[] = {
    /* the RetroPad as RetroArch maps the keyboard (x z s a q w), plus the keys a child finds */
    {SDL_SCANCODE_UP, RETRO_DEVICE_ID_JOYPAD_UP},
    {SDL_SCANCODE_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN},
    {SDL_SCANCODE_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT},
    {SDL_SCANCODE_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT},
    {SDL_SCANCODE_RETURN, RETRO_DEVICE_ID_JOYPAD_A}, /* the red button */
    {SDL_SCANCODE_KP_ENTER, RETRO_DEVICE_ID_JOYPAD_A},
    {SDL_SCANCODE_SPACE, RETRO_DEVICE_ID_JOYPAD_A},
    {SDL_SCANCODE_X, RETRO_DEVICE_ID_JOYPAD_A},
    {SDL_SCANCODE_BACKSPACE, RETRO_DEVICE_ID_JOYPAD_B}, /* the yellow one */
    {SDL_SCANCODE_Z, RETRO_DEVICE_ID_JOYPAD_B},
    {SDL_SCANCODE_S, RETRO_DEVICE_ID_JOYPAD_X},
    {SDL_SCANCODE_A, RETRO_DEVICE_ID_JOYPAD_Y},
    {SDL_SCANCODE_Q, RETRO_DEVICE_ID_JOYPAD_L},
    {SDL_SCANCODE_PAGEUP, RETRO_DEVICE_ID_JOYPAD_L},
    {SDL_SCANCODE_W, RETRO_DEVICE_ID_JOYPAD_R},
    {SDL_SCANCODE_PAGEDOWN, RETRO_DEVICE_ID_JOYPAD_R},
    {SDL_SCANCODE_ESCAPE, RETRO_DEVICE_ID_JOYPAD_START}, /* held: the pause */
    {SDL_SCANCODE_P, RETRO_DEVICE_ID_JOYPAD_START},
    {SDL_SCANCODE_RSHIFT, RETRO_DEVICE_ID_JOYPAD_SELECT},
};

/* The gamepad's face buttons, two ways. By position, as on the RF35H: red on the right, yellow below,
   blue on top, green on the left (a Super Nintendo style pad wears those very colours). By colour, on
   the pads that wear the Xbox ones (red B on the right, yellow Y on top, green A below, blue X on the
   left): the voice says "the yellow button", so there the yellow one it is. The sticks stay out, as in
   the core: a drifting stick must not choose for her. */
#define PAD_ROWS 12
typedef struct {
    int button;
    unsigned id;
} pad_row;
#define PAD_REST                                                                                              \
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, RETRO_DEVICE_ID_JOYPAD_L},                                          \
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, RETRO_DEVICE_ID_JOYPAD_R},                                     \
        {SDL_CONTROLLER_BUTTON_START, RETRO_DEVICE_ID_JOYPAD_START},                                         \
        {SDL_CONTROLLER_BUTTON_BACK, RETRO_DEVICE_ID_JOYPAD_SELECT},                                         \
        {SDL_CONTROLLER_BUTTON_DPAD_UP, RETRO_DEVICE_ID_JOYPAD_UP},                                          \
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN},                                      \
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT},                                      \
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT}
static const pad_row PAD_BY_POSITION[PAD_ROWS] = {
    {SDL_CONTROLLER_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_A}, /* right: red */
    {SDL_CONTROLLER_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_B}, /* below: yellow */
    {SDL_CONTROLLER_BUTTON_Y, RETRO_DEVICE_ID_JOYPAD_X}, /* on top: blue */
    {SDL_CONTROLLER_BUTTON_X, RETRO_DEVICE_ID_JOYPAD_Y}, /* left: green */
    PAD_REST,
};
static const pad_row PAD_BY_COLOUR[PAD_ROWS] = {
    {SDL_CONTROLLER_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_A}, /* red B */
    {SDL_CONTROLLER_BUTTON_Y, RETRO_DEVICE_ID_JOYPAD_B}, /* yellow Y */
    {SDL_CONTROLLER_BUTTON_X, RETRO_DEVICE_ID_JOYPAD_X}, /* blue X */
    {SDL_CONTROLLER_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_Y}, /* green A */
    PAD_REST,
};
enum { PAD_AUTO, PAD_COLOUR, PAD_POSITION };
static int s_pad_mode = PAD_AUTO; /* --pad */
static bool s_pad_colour[MAX_PADS];

static const pad_row *pad_map(int slot) { return s_pad_colour[slot] ? PAD_BY_COLOUR : PAD_BY_POSITION; }

static void log_cb(enum retro_log_level level, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (level == RETRO_LOG_DEBUG && !s_verbose)
        return;
    if (s_log) {
        fputs(buf, s_log);
        fflush(s_log); /* (a line or two a second at most: the log is whole even after a crash) */
    }
    if (level >= RETRO_LOG_WARN || s_verbose)
        fputs(buf, stderr);
}

static bool rumble_cb(unsigned port, enum retro_rumble_effect effect, uint16_t strength)
{
    (void)port;
    (void)effect;
    if (!S.SDL_GameControllerRumble)
        return false;
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i])
            S.SDL_GameControllerRumble(s_pads[i], strength, strength, strength ? 1000 : 0);
    return true;
}

static bool env_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((struct retro_log_callback *)data)->log = log_cb;
        return true;
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(const enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char **)data = s_sys_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = s_save_dir;
        return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true;
        return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        return true;
    case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        ((struct retro_rumble_interface *)data)->set_rumble_state = rumble_cb;
        return true;
    case RETRO_ENVIRONMENT_SHUTDOWN: /* Esci, or the goodnight at the end of the session */
        s_quit = true;
        return true;
    default:
        return false;
    }
}

static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch)
{
    (void)w;
    (void)h;
    if (!data) /* the same picture as before: the texture already holds it */
        return;
    S.SDL_UpdateTexture(s_texture, NULL, data, (int)pitch);
    s_new_frame = true;
}

static void audio_sample_cb(int16_t l, int16_t r)
{
    (void)l;
    (void)r;
}

static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    if (s_audio)
        S.SDL_QueueAudio(s_audio, data, (Uint32)(frames * 4));
    return frames;
}

static void input_poll_cb(void)
{
    uint32_t bits = s_latched; /* a tap shorter than a frame still counts, for one frame */
    s_latched = 0;
    const Uint8 *keys = S.SDL_GetKeyboardState(NULL);
    if (!(S.SDL_GetModState() & (KMOD_ALT | KMOD_GUI))) /* (Alt+Tab, Alt+F4, Cmd+Q: not for the game) */
        for (size_t i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); i++)
            if (keys[KEYS[i].key])
                bits |= 1u << KEYS[i].id;
    for (int p = 0; p < MAX_PADS; p++)
        if (s_pads[p]) {
            const pad_row *map = pad_map(p);
            for (int i = 0; i < PAD_ROWS; i++)
                if (S.SDL_GameControllerGetButton(s_pads[p], map[i].button))
                    bits |= 1u << map[i].id;
        }
    s_bits = bits;
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port || device != RETRO_DEVICE_JOYPAD || index)
        return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK)
        return (int16_t)s_bits;
    return (int16_t)(id < 16 && ((s_bits >> id) & 1));
}

/* ------------------------------------------------------------------ window, pads */
static bool s_fullscreen;

static void set_fullscreen(bool on)
{
    if (S.SDL_SetWindowFullscreen(s_window, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0)
        s_fullscreen = on;
    else
        note("full screen refused: %s", S.SDL_GetError());
    S.SDL_ShowCursor(s_fullscreen ? SDL_DISABLE : SDL_ENABLE);
    note("%s", s_fullscreen ? "full screen" : "window");
}

static void set_icon(void)
{
    char p[PATHLEN];
    const char *places[] = {"deva-adventures.png", "../Resources/deva-adventures.png",
                            "../share/icons/hicolor/256x256/apps/deva-adventures.png"};
    for (size_t i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
        if (!fit(p, sizeof(p), "%s/%s", s_exe_dir, places[i]))
            continue;
        int w, h, comp;
        unsigned char *px = stbi_load(p, &w, &h, &comp, 4);
        if (!px)
            continue;
        SDL_Surface *icon = S.SDL_CreateRGBSurfaceWithFormatFrom(px, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
        if (icon) {
            S.SDL_SetWindowIcon(s_window, icon);
            S.SDL_FreeSurface(icon);
        }
        stbi_image_free(px);
        return;
    }
}

static void pad_added(int device_index)
{
    if (!S.SDL_IsGameController(device_index))
        return;
    SDL_GameController *c = S.SDL_GameControllerOpen(device_index);
    if (!c)
        return;
    SDL_JoystickID id = S.SDL_JoystickInstanceID(S.SDL_GameControllerGetJoystick(c));
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i] && S.SDL_JoystickInstanceID(S.SDL_GameControllerGetJoystick(s_pads[i])) == id) {
            S.SDL_GameControllerClose(c); /* (already open: SDL counts the opens) */
            return;
        }
    for (int i = 0; i < MAX_PADS; i++)
        if (!s_pads[i]) {
            bool colour = s_pad_mode == PAD_COLOUR;
            if (s_pad_mode == PAD_AUTO && S.SDL_GameControllerGetType) {
                SDL_GameControllerType t = S.SDL_GameControllerGetType(c);
                colour = t == SDL_CONTROLLER_TYPE_XBOX360 || t == SDL_CONTROLLER_TYPE_XBOXONE;
            }
            s_pads[i] = c;
            s_pad_colour[i] = colour;
            note("gamepad %d connected: %s", (int)id,
                 colour ? "buttons by colour, as on an Xbox pad (yellow Y on top)"
                        : "buttons by position, as on the console (yellow below)");
            return;
        }
    S.SDL_GameControllerClose(c);
}

static int pad_slot(SDL_JoystickID id)
{
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i] && S.SDL_JoystickInstanceID(S.SDL_GameControllerGetJoystick(s_pads[i])) == id)
            return i;
    return -1;
}

static void pad_removed(SDL_JoystickID id)
{
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i] && S.SDL_JoystickInstanceID(S.SDL_GameControllerGetJoystick(s_pads[i])) == id) {
            S.SDL_GameControllerClose(s_pads[i]);
            s_pads[i] = NULL;
            note("gamepad %d disconnected", (int)id);
        }
}

static void present(void)
{
    S.SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    S.SDL_RenderClear(s_renderer);
    S.SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    S.SDL_RenderPresent(s_renderer);
}

static void latch_key(int scancode)
{
    for (size_t i = 0; i < sizeof(KEYS) / sizeof(KEYS[0]); i++)
        if (KEYS[i].key == scancode)
            s_latched |= 1u << KEYS[i].id;
}

static void latch_pad(SDL_JoystickID id, int button)
{
    int slot = pad_slot(id);
    if (slot < 0)
        return;
    const pad_row *map = pad_map(slot);
    for (int i = 0; i < PAD_ROWS; i++)
        if (map[i].button == button)
            s_latched |= 1u << map[i].id;
}

/* ------------------------------------------------------------------ main */
static void usage(FILE *f)
{
#if defined(_WIN32)
    const char *full_keys = "F11 o Alt+Invio", *saves = "%APPDATA%\\deva-adventures";
#elif defined(__APPLE__)
    const char *full_keys = "F11 o Cmd+F", *saves = "~/Library/Application Support/deva-adventures";
#else
    const char *full_keys = "F11 o Alt+Invio", *saves = "~/.local/share/deva-adventures";
#endif
    fprintf(f,
            "Deva's Awesome Adventures %s - il gioco sul PC, senza RetroArch.\n\n"
            "  deva-adventures                 in una finestra (%s: schermo intero e ritorno)\n"
            "  deva-adventures --fullscreen    a schermo intero\n"
            "  --scale N                       finestra N volte 320x240 (di serie: la più grande che sta)\n"
            "  --data CARTELLA                 i dati del gioco (deva_adventures o la cartella che la contiene)\n"
            "  --saves CARTELLA                i salvataggi (di serie %s)\n"
            "  --no-audio                      senza suono\n"
            "  --no-pause                      non fermare il gioco quando la finestra non è davanti\n"
            "  --pad colori | posizione        i tasti del gamepad per colore (come su Xbox) o per posizione\n"
            "                                  (come sulla console); di serie per colore sui pad tipo Xbox\n"
            "  --verbose                       tutti i messaggi del gioco, anche sul terminale\n"
            "  --check                         controlla soltanto che ci siano i dati e SDL2, poi esce\n\n"
            "Tastiera: frecce = croce, Invio o Spazio = rosso, Backspace = giallo, A = verde, S = blu,\n"
            "Q e W = L e R (per i grandi: tenuti insieme 2 secondi), Esc tenuto = pausa. Gamepad: rosso a destra;\n"
            "giallo in alto sui pad tipo Xbox (il tasto Y), in basso sugli altri, come sulla console.\n"
            "Per chiudere: Esci nel gioco, o la finestra.\n",
            GAME_VERSION, full_keys, saves);
}

int main(int argc, char **argv)
{
    os_init();
    setvbuf(stderr, NULL, _IONBF, 0); /* (Windows buffers it when it goes to a file) */
    bool want_full = false, want_audio = true, pause_unfocused = true, check_only = false;
    int scale = 0;
    const char *data_arg = NULL, *saves_arg = NULL;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        bool valued = !strcmp(a, "--scale") || !strcmp(a, "--data") || !strcmp(a, "--saves") || !strcmp(a, "--pad");
        if (valued && i + 1 >= argc) {
            fprintf(stderr, "%s: %s vuole un valore\n\n", APP, a);
            usage(stderr);
            return 2;
        }
        if (!strcmp(a, "-f") || !strcmp(a, "--fullscreen"))
            want_full = true;
        else if (!strcmp(a, "-w") || !strcmp(a, "--window"))
            want_full = false;
        else if (!strcmp(a, "--scale"))
            scale = atoi(argv[++i]);
        else if (!strcmp(a, "--data"))
            data_arg = argv[++i];
        else if (!strcmp(a, "--saves"))
            saves_arg = argv[++i];
        else if (!strcmp(a, "--check"))
            check_only = s_no_dialog = true;
        else if (!strcmp(a, "--pad")) {
            const char *m = argv[++i];
            if (!strcmp(m, "colori") || !strcmp(m, "colors"))
                s_pad_mode = PAD_COLOUR;
            else if (!strcmp(m, "posizione") || !strcmp(m, "position"))
                s_pad_mode = PAD_POSITION;
            else if (strcmp(m, "auto")) {
                fprintf(stderr, "%s: --pad vuole colori, posizione o auto (non %s)\n", APP, m);
                return 2;
            }
        }
        else if (!strcmp(a, "--no-audio"))
            want_audio = false;
        else if (!strcmp(a, "--no-pause"))
            pause_unfocused = false;
        else if (!strcmp(a, "--verbose"))
            s_verbose = true;
        else if (!strcmp(a, "--version")) {
            printf("%s %s\n", APP, GAME_VERSION);
            return 0;
        } else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else {
            fprintf(stderr, "%s: opzione sconosciuta: %s\n\n", APP, a);
            usage(stderr);
            return 2;
        }
    }
    if (scale < 0 || scale > 12)
        scale = 0;

    find_exe_dir();
    char why[512];
    bool sdl_ok = load_sdl(why, sizeof(why));
    if (!find_data(data_arg) || (!check_only && !find_saves(saves_arg)))
        return 3;
    if (!check_only && !take_lock()) {
        tell_user(false, "Il gioco è già aperto.");
        return 5;
    }
    char path[PATHLEN], old[PATHLEN];
    if (!check_only && fit(path, sizeof(path), "%s/deva-adventures.log", s_save_dir)) {
        if (fit(old, sizeof(old), "%s.1", path))
            plat_replace(path, old); /* (the run before, for whoever looks into a problem) */
        s_log = os_fopen(path, "w");
    }
    if (!sdl_ok) {
#if defined(_WIN32)
        tell_user(true, "Manca SDL2.dll accanto al programma (%s): scompatta di nuovo tutto il pacchetto, oppure "
                   "reinstalla il gioco.", why);
#elif defined(__APPLE__)
        tell_user(true, "Manca la libreria SDL2 dell'app (%s): scarica di nuovo il gioco (il file .dmg) e copialo di "
                   "nuovo in Applicazioni.", why);
#else
        tell_user(true, "Serve la libreria SDL2 (%s). Installala: sudo apt install libsdl2-2.0-0 (Debian, Ubuntu, "
                   "Mint), sudo dnf install SDL2 (Fedora), sudo pacman -S sdl2-compat (Arch).", why);
#endif
        return 4;
    }
    if (check_only) {
        SDL_version v;
        S.SDL_GetVersion(&v);
        printf("%s %s: dati in %s" OS_SEP "deva_adventures, SDL %d.%d.%d: tutto pronto.\n", APP, GAME_VERSION,
               s_sys_dir, v.major, v.minor, v.patch);
        return 0;
    }
    note("Deva's Awesome Adventures %s: data %s" OS_SEP "deva_adventures, saves %s", GAME_VERSION, s_sys_dir,
         s_save_dir);

#if !defined(_WIN32) && !defined(__APPLE__)
    /* the window's class (X11) and app id (Wayland) as the launcher's name: the dock shows our icon */
    os_default_env("SDL_VIDEO_X11_WMCLASS", APP);
    os_default_env("SDL_VIDEO_WAYLAND_WMCLASS", APP);
#endif
    /* Windows: pixels as they are, not stretched by the display's scaling (blurred) */
    S.SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "permonitorv2");
    /* face buttons by position on every pad (Nintendo ones too); crisp pixels; the desktop stays composited */
    S.SDL_SetHint("SDL_GAMECONTROLLER_USE_BUTTON_LABELS", "0");
    S.SDL_SetHint("SDL_RENDER_SCALE_QUALITY", "0");
    S.SDL_SetHint("SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR", "0");
    S.SDL_SetHint("SDL_APP_NAME", TITLE);
    S.SDL_SetHint("SDL_AUDIO_DEVICE_APP_NAME", TITLE);
    S.SDL_SetHint("SDL_AUDIO_DEVICE_STREAM_ROLE", "game");
    if (S.SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0 &&
        S.SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        tell_user(true, "Non riesco ad aprire lo schermo: %s", S.SDL_GetError());
        return 4;
    }

    if (!scale) {
        SDL_Rect r;
        scale = 2;
        if (S.SDL_GetDisplayUsableBounds(0, &r) == 0)
            for (int s = 6; s >= 2; s--)
                if (SCREEN_W * s <= r.w * 9 / 10 && SCREEN_H * s <= r.h * 9 / 10) {
                    scale = s;
                    break;
                }
    }
    s_window = S.SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W * scale,
                                  SCREEN_H * scale, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
    if (s_window)
        s_renderer = S.SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED);
    if (s_window && !s_renderer)
        s_renderer = S.SDL_CreateRenderer(s_window, -1, SDL_RENDERER_SOFTWARE);
    if (s_renderer)
        s_texture = S.SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, SCREEN_W,
                                        SCREEN_H);
    if (!s_texture) {
        tell_user(true, "Non riesco ad aprire la finestra del gioco: %s", S.SDL_GetError());
        S.SDL_Quit();
        return 4;
    }
    S.SDL_SetWindowMinimumSize(s_window, SCREEN_W, SCREEN_H);
    S.SDL_RenderSetLogicalSize(s_renderer, SCREEN_W, SCREEN_H);
    S.SDL_RenderSetIntegerScale(s_renderer, SDL_TRUE);
    SDL_RendererInfo ri;
    const char *vd = S.SDL_GetCurrentVideoDriver();
    if (S.SDL_GetRendererInfo(s_renderer, &ri) == 0) /* (for whoever looks into a black or slow screen) */
        note("video %s, renderer %s%s, window %dx%d", vd ? vd : "?", ri.name ? ri.name : "?",
             ri.flags & SDL_RENDERER_ACCELERATED ? "" : " (software)", SCREEN_W * scale, SCREEN_H * scale);
    set_icon();
    if (want_full)
        set_fullscreen(true);

    Uint32 chunk = 1024 * 4; /* the bytes of sound the speaker takes at a time */
    if (want_audio) {
        if (S.SDL_Init(SDL_INIT_AUDIO) == 0) {
            SDL_AudioSpec want, have;
            memset(&want, 0, sizeof(want));
            memset(&have, 0, sizeof(have));
            want.freq = AUDIO_RATE;
            want.format = AUDIO_S16SYS;
            want.channels = 2;
            want.samples = 1024;
            s_audio = S.SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
            if (s_audio && have.samples)
                chunk = have.samples * 4u;
            if (s_audio) {
                const char *ad = S.SDL_GetCurrentAudioDriver();
                note("sound %s, %d Hz, %u bytes at a time", ad ? ad : "?", have.freq, (unsigned)chunk);
            }
        }
        if (!s_audio)
            tell_user(false, "Non riesco ad aprire l'audio (%s): il gioco parte senza voce né musica.", S.SDL_GetError());
    }

    retro_set_environment(env_cb);
    retro_init();
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_sample_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
    if (!retro_load_game(NULL)) {
        tell_user(true, "Non riesco a caricare il gioco: i dettagli sono in %s/deva-adventures.log.", s_save_dir);
        retro_deinit();
        S.SDL_Quit();
        return 3;
    }
    S.SDL_ShowWindow(s_window);

    /* The clock runs the frames, one every 1/60 s, so that every picture stays on screen for the same
       time (frames run whenever the speaker wants sound would come in bursts, one in four never seen).
       The sound keeps the clock honest. It starts once `fill` bytes are queued; the speaker takes `chunk`
       at a time and must never find less, so a low queue brings the next frame forward; a queue that
       grows (the two clocks drift apart) skips a beat, at most one a second. */
    const Uint64 freq = S.SDL_GetPerformanceFrequency(), period = freq / FPS;
    const Uint32 frame_bytes = SAMPLES_PER_FRAME * 4, fill = chunk + 2 * frame_bytes,
                 too_much = fill + chunk + 3 * frame_bytes;
    Uint64 next = S.SDL_GetPerformanceCounter(), started = next;
    bool focused = true, minimized = false, redraw = true, was_running = true, sound_on = false;
    Uint64 last_ran = 0; /* when the last frame ran: a frame later than 1.5 periods is a hitch */
    long frames = 0, late = 0, bursts = 0, skipped = 0, last_skip = -FPS, paused_loops = 0, stuck = 0;
    const char *tf = getenv("DEVA_TEST_FRAMES");
    long test_frames = tf ? atol(tf) : 0;
    while (!s_quit) {
        SDL_Event e;
        while (S.SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                s_quit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                case SDL_WINDOWEVENT_FOCUS_LOST: focused = false; break;
                case SDL_WINDOWEVENT_FOCUS_GAINED: focused = true; break;
                case SDL_WINDOWEVENT_MINIMIZED:
                case SDL_WINDOWEVENT_HIDDEN: minimized = true; break;
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_SHOWN: minimized = false; redraw = true; break;
                case SDL_WINDOWEVENT_EXPOSED:
                case SDL_WINDOWEVENT_SIZE_CHANGED: redraw = true; break;
                default: break;
                }
                break;
            case SDL_KEYDOWN: {
                if (e.key.repeat)
                    break;
                int sc = e.key.keysym.scancode, mod = e.key.keysym.mod;
                bool enter = sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_KP_ENTER;
                /* full screen and back: F11, Alt+Enter (Windows), Cmd+F (Mac); none of them is for the game */
                if (sc == SDL_SCANCODE_F11 || ((mod & KMOD_ALT) && enter) || ((mod & KMOD_GUI) && sc == SDL_SCANCODE_F)) {
                    set_fullscreen(!s_fullscreen);
                    redraw = true;
                } else if (!(mod & (KMOD_ALT | KMOD_GUI))) {
                    latch_key(sc);
                }
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN:
                latch_pad(e.cbutton.which, e.cbutton.button);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                pad_added(e.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                pad_removed(e.cdevice.which);
                break;
            case SDL_AUDIODEVICEREMOVED: /* (headphones unplugged, the sound server gone) */
                if (s_audio && !e.adevice.iscapture && e.adevice.which == s_audio) {
                    S.SDL_CloseAudioDevice(s_audio);
                    s_audio = 0;
                    sound_on = false;
                    note("the sound device is gone: the game goes on without sound");
                }
                break;
            default:
                break;
            }
        }
        if (s_quit)
            break;
        bool running = !minimized && (focused || !pause_unfocused);
        if (running != was_running) { /* the game waits while the window is not in front */
            if (!running && sound_on) {
                S.SDL_PauseAudioDevice(s_audio, 1);
                sound_on = false;
            }
            note(running ? "back in front: the game goes on" : "window not in front: the game waits");
            was_running = running;
            next = S.SDL_GetPerformanceCounter();
            last_ran = 0;
        }
        if (!running) {
            if (redraw) {
                present();
                redraw = false;
            }
            paused_loops++;
            S.SDL_Delay(20);
            continue;
        }
        Uint64 now = S.SDL_GetPerformanceCounter();
        if (now > next + 10 * period)
            next = now; /* (after a long stop: no racing to catch up) */
        if (sound_on) {
            Uint32 queued = S.SDL_GetQueuedAudioSize(s_audio);
            if (queued > AUDIO_RATE * 4u) { /* a whole second behind: the speaker stopped taking sound */
                S.SDL_ClearQueuedAudio(s_audio);
                if (!stuck++)
                    note("the sound device takes no sound: the queue emptied");
            } else if (queued < chunk && next > now) {
                next = now; /* the speaker would soon find too little: this frame now */
            } else if (queued > too_much && now >= next && frames - last_skip >= FPS) {
                next += period; /* the sound lags behind the picture: skip a beat */
                last_skip = frames;
                skipped++;
            }
        }
        int ran = 0;
        while (ran < 3 && !s_quit && now >= next) {
            retro_run();
            ran++;
            next += period;
        }
        if (ran) {
            late += last_ran && now - last_ran > period * 3 / 2;
            last_ran = now;
        }
        frames += ran;
        bursts += ran > 1;
        if (test_frames > 0 && frames >= test_frames)
            s_quit = true; /* (DEVA_TEST_FRAMES: a smoke test ends here, in order) */
        if (s_audio && !sound_on && S.SDL_GetQueuedAudioSize(s_audio) >= fill) {
            S.SDL_PauseAudioDevice(s_audio, 0); /* enough sound queued: the speaker starts */
            sound_on = true;
        }
        if (s_new_frame || redraw) {
            present();
            s_new_frame = redraw = false;
        }
        now = S.SDL_GetPerformanceCounter();
        if (next > now) {
            Uint64 ms = (next - now) * 1000 / freq;
            S.SDL_Delay(ms < 1 ? 1 : ms > 10 ? 10 : (Uint32)ms);
        }
    }

    double secs = (double)(S.SDL_GetPerformanceCounter() - started) / (double)freq;
    note("%ld frames in %.1f s, about %ld s of them waiting (window not in front); %ld late (over 25 ms after "
         "the one before), %ld times two or more at once, %ld beats skipped for the sound",
         frames, secs, paused_loops / 50, late, bursts, skipped);
    retro_unload_game(); /* the profile and the logs, flushed to the disk */
    retro_deinit();
    if (s_audio)
        S.SDL_CloseAudioDevice(s_audio);
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i])
            S.SDL_GameControllerClose(s_pads[i]);
    S.SDL_DestroyTexture(s_texture);
    S.SDL_DestroyRenderer(s_renderer);
    S.SDL_DestroyWindow(s_window);
    S.SDL_Quit();
    if (s_log)
        fclose(s_log);
    return 0;
}
