/* Deva's Awesome Adventures - the part of the SDL2 API the PC program uses (1.1).
 *
 * Declared here so that the program builds with a bare C compiler (no SDL headers) and loads
 * libSDL2-2.0.so.0 at run time: SDL2 keeps its ABI stable across all 2.x releases (and sdl2-compat
 * keeps it on top of SDL3). Every value and layout below is checked against the real SDL2 headers
 * by tools/harness/check_sdl2_api.sh. Needs SDL 2.0.9 or newer (2018); rumble and the gamepad's type
 * (2.0.12) are optional. */
#ifndef DEVA_SDL2_API_H
#define DEVA_SDL2_API_H

#include <stdint.h>

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int32_t Sint32;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

typedef struct SDL_version {
    Uint8 major, minor, patch;
} SDL_version;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Surface SDL_Surface;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct _SDL_GameController SDL_GameController;

typedef struct SDL_Rect {
    int x, y, w, h;
} SDL_Rect;

typedef struct SDL_RendererInfo {
    const char *name;
    Uint32 flags;
    Uint32 num_texture_formats;
    Uint32 texture_formats[16];
    int max_texture_width;
    int max_texture_height;
} SDL_RendererInfo;

typedef Uint16 SDL_AudioFormat;
typedef Uint32 SDL_AudioDeviceID;
typedef Sint32 SDL_JoystickID;
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);

typedef struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint8 silence;
    Uint16 samples;
    Uint16 padding;
    Uint32 size;
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

typedef struct SDL_Keysym {
    int scancode; /* SDL_Scancode */
    Sint32 sym;   /* SDL_Keycode */
    Uint16 mod;
    Uint32 unused;
} SDL_Keysym;

typedef struct SDL_WindowEvent {
    Uint32 type, timestamp, windowID;
    Uint8 event, padding1, padding2, padding3;
    Sint32 data1, data2;
} SDL_WindowEvent;

typedef struct SDL_KeyboardEvent {
    Uint32 type, timestamp, windowID;
    Uint8 state, repeat, padding2, padding3;
    SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_ControllerDeviceEvent {
    Uint32 type, timestamp;
    Sint32 which;
} SDL_ControllerDeviceEvent;

typedef struct SDL_ControllerButtonEvent {
    Uint32 type, timestamp;
    SDL_JoystickID which;
    Uint8 button, state, padding1, padding2;
} SDL_ControllerButtonEvent;

typedef struct SDL_AudioDeviceEvent {
    Uint32 type, timestamp, which;
    Uint8 iscapture, padding1, padding2, padding3;
} SDL_AudioDeviceEvent;

typedef union SDL_Event {
    Uint32 type;
    SDL_WindowEvent window;
    SDL_KeyboardEvent key;
    SDL_ControllerDeviceEvent cdevice;
    SDL_ControllerButtonEvent cbutton;
    SDL_AudioDeviceEvent adevice;
    void *align; /* (the real union holds pointers) */
    Uint8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
} SDL_Event;

#define SDL_INIT_AUDIO 0x00000010u
#define SDL_INIT_VIDEO 0x00000020u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_INIT_EVENTS 0x00004000u

enum {
    SDL_QUIT = 0x100,
    SDL_WINDOWEVENT = 0x200,
    SDL_KEYDOWN = 0x300,
    SDL_KEYUP = 0x301,
    SDL_CONTROLLERBUTTONDOWN = 0x651,
    SDL_CONTROLLERBUTTONUP = 0x652,
    SDL_CONTROLLERDEVICEADDED = 0x653,
    SDL_CONTROLLERDEVICEREMOVED = 0x654,
    SDL_AUDIODEVICEREMOVED = 0x1101,
};

enum {
    SDL_WINDOWEVENT_SHOWN = 1,
    SDL_WINDOWEVENT_HIDDEN = 2,
    SDL_WINDOWEVENT_EXPOSED = 3,
    SDL_WINDOWEVENT_SIZE_CHANGED = 6,
    SDL_WINDOWEVENT_MINIMIZED = 7,
    SDL_WINDOWEVENT_RESTORED = 9,
    SDL_WINDOWEVENT_FOCUS_GAINED = 12,
    SDL_WINDOWEVENT_FOCUS_LOST = 13,
};

typedef enum { /* (the real one has the same values) */
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200,
    KMOD_LGUI = 0x0400,
    KMOD_RGUI = 0x0800,
    KMOD_NUM = 0x1000,
    KMOD_CAPS = 0x2000,
    KMOD_MODE = 0x4000,
    KMOD_SCROLL = 0x8000,
} SDL_Keymod;
#define KMOD_ALT (KMOD_LALT | KMOD_RALT)
#define KMOD_GUI (KMOD_LGUI | KMOD_RGUI)

enum {
    SDL_SCANCODE_A = 4,
    SDL_SCANCODE_F = 9,
    SDL_SCANCODE_P = 19,
    SDL_SCANCODE_Q = 20,
    SDL_SCANCODE_S = 22,
    SDL_SCANCODE_W = 26,
    SDL_SCANCODE_X = 27,
    SDL_SCANCODE_Z = 29,
    SDL_SCANCODE_RETURN = 40,
    SDL_SCANCODE_ESCAPE = 41,
    SDL_SCANCODE_BACKSPACE = 42,
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_F11 = 68,
    SDL_SCANCODE_PAGEUP = 75,
    SDL_SCANCODE_PAGEDOWN = 78,
    SDL_SCANCODE_RIGHT = 79,
    SDL_SCANCODE_LEFT = 80,
    SDL_SCANCODE_DOWN = 81,
    SDL_SCANCODE_UP = 82,
    SDL_SCANCODE_KP_ENTER = 88,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_NUM_SCANCODES = 512,
};

typedef enum { /* (the real one goes on; only the Xbox ones matter here) */
    SDL_CONTROLLER_TYPE_UNKNOWN = 0,
    SDL_CONTROLLER_TYPE_XBOX360 = 1,
    SDL_CONTROLLER_TYPE_XBOXONE = 2,
} SDL_GameControllerType;

enum {
    SDL_CONTROLLER_BUTTON_A = 0,
    SDL_CONTROLLER_BUTTON_B = 1,
    SDL_CONTROLLER_BUTTON_X = 2,
    SDL_CONTROLLER_BUTTON_Y = 3,
    SDL_CONTROLLER_BUTTON_BACK = 4,
    SDL_CONTROLLER_BUTTON_START = 6,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER = 9,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER = 10,
    SDL_CONTROLLER_BUTTON_DPAD_UP = 11,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN = 12,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT = 13,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT = 14,
};

#define SDL_PIXELFORMAT_RGB565 0x15151002u
#define SDL_PIXELFORMAT_RGBA32 0x16762004u /* (little endian: ABGR8888, bytes R G B A) */
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_WINDOW_HIDDEN 0x00000008u
#define SDL_WINDOW_RESIZABLE 0x00000020u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u
#define SDL_WINDOWPOS_CENTERED 0x2FFF0000
#define SDL_RENDERER_SOFTWARE 0x00000001u
#define SDL_RENDERER_ACCELERATED 0x00000002u
#define SDL_RENDERER_PRESENTVSYNC 0x00000004u
#define SDL_MESSAGEBOX_ERROR 0x00000010u
#define SDL_MESSAGEBOX_WARNING 0x00000020u
#define SDL_MESSAGEBOX_INFORMATION 0x00000040u
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define AUDIO_S16SYS 0x8010 /* little endian; the program is built for x86_64 and aarch64 */

#include "sdl2_functions.h"

#endif
