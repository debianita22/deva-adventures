/* Deva's Awesome Adventures - platform/sdl/sdl2_api.h held against the real SDL2 headers (1.1).
 *
 * Built twice by check_sdl2_api.sh: with -DREAL_SDL (the real <SDL.h>: every function the PC program
 * loads is assigned to a pointer of the type the program uses, so a wrong prototype does not
 * compile) and with the program's own header. Both print every value and layout the program relies
 * on; the two lists must be the same. */
#include <stddef.h>
#include <stdio.h>

#ifdef REAL_SDL
#include <SDL.h>
#include "../../platform/sdl/sdl2_functions.h"
#define X(ret, name, args) __attribute__((unused)) static ret(*check_##name) args = name;
SDL2_FUNCTIONS(X)
SDL2_OPTIONAL_FUNCTIONS(X)
#undef X
#else
#include "../../platform/sdl/sdl2_api.h"
#endif

#define P(x) printf("%-44s %lld\n", #x, (long long)(x));

int main(void)
{
    P(SDL_INIT_AUDIO) P(SDL_INIT_VIDEO) P(SDL_INIT_GAMECONTROLLER) P(SDL_INIT_EVENTS)
    P(SDL_QUIT) P(SDL_WINDOWEVENT) P(SDL_KEYDOWN) P(SDL_KEYUP) P(SDL_CONTROLLERBUTTONDOWN)
    P(SDL_CONTROLLERBUTTONUP) P(SDL_CONTROLLERDEVICEADDED) P(SDL_CONTROLLERDEVICEREMOVED) P(SDL_AUDIODEVICEREMOVED)
    P(SDL_WINDOWEVENT_SHOWN) P(SDL_WINDOWEVENT_HIDDEN) P(SDL_WINDOWEVENT_EXPOSED) P(SDL_WINDOWEVENT_SIZE_CHANGED)
    P(SDL_WINDOWEVENT_MINIMIZED) P(SDL_WINDOWEVENT_RESTORED) P(SDL_WINDOWEVENT_FOCUS_GAINED)
    P(SDL_WINDOWEVENT_FOCUS_LOST)
    P(SDL_SCANCODE_A) P(SDL_SCANCODE_F) P(SDL_SCANCODE_P) P(SDL_SCANCODE_Q) P(SDL_SCANCODE_S) P(SDL_SCANCODE_W) P(SDL_SCANCODE_X)
    P(SDL_SCANCODE_Z) P(SDL_SCANCODE_RETURN) P(SDL_SCANCODE_ESCAPE) P(SDL_SCANCODE_BACKSPACE) P(SDL_SCANCODE_SPACE)
    P(SDL_SCANCODE_F11) P(SDL_SCANCODE_PAGEUP) P(SDL_SCANCODE_PAGEDOWN) P(SDL_SCANCODE_RIGHT) P(SDL_SCANCODE_LEFT)
    P(SDL_SCANCODE_DOWN) P(SDL_SCANCODE_UP) P(SDL_SCANCODE_KP_ENTER) P(SDL_SCANCODE_RSHIFT) P(SDL_NUM_SCANCODES)
    P(SDL_CONTROLLER_BUTTON_A) P(SDL_CONTROLLER_BUTTON_B) P(SDL_CONTROLLER_BUTTON_X) P(SDL_CONTROLLER_BUTTON_Y)
    P(SDL_CONTROLLER_BUTTON_BACK) P(SDL_CONTROLLER_BUTTON_START) P(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
    P(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) P(SDL_CONTROLLER_BUTTON_DPAD_UP) P(SDL_CONTROLLER_BUTTON_DPAD_DOWN)
    P(SDL_CONTROLLER_BUTTON_DPAD_LEFT) P(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
    P(SDL_CONTROLLER_TYPE_UNKNOWN) P(SDL_CONTROLLER_TYPE_XBOX360) P(SDL_CONTROLLER_TYPE_XBOXONE)
    P(KMOD_LSHIFT) P(KMOD_RSHIFT) P(KMOD_LCTRL) P(KMOD_RCTRL) P(KMOD_LALT) P(KMOD_RALT) P(KMOD_LGUI) P(KMOD_RGUI)
    P(KMOD_NUM) P(KMOD_CAPS) P(KMOD_MODE) P(KMOD_SCROLL) P(KMOD_ALT) P(KMOD_GUI) P(sizeof(SDL_Keymod))
    P(sizeof(SDL_GameControllerType))
    P(SDL_PIXELFORMAT_RGB565) P(SDL_PIXELFORMAT_RGBA32) P(SDL_TEXTUREACCESS_STREAMING)
    P(SDL_WINDOW_HIDDEN) P(SDL_WINDOW_RESIZABLE) P(SDL_WINDOW_FULLSCREEN_DESKTOP) P(SDL_WINDOW_ALLOW_HIGHDPI)
    P(SDL_WINDOWPOS_CENTERED) P(SDL_RENDERER_SOFTWARE) P(SDL_RENDERER_ACCELERATED) P(SDL_RENDERER_PRESENTVSYNC)
    P(SDL_MESSAGEBOX_ERROR) P(SDL_MESSAGEBOX_WARNING) P(SDL_MESSAGEBOX_INFORMATION) P(SDL_ENABLE) P(SDL_DISABLE)
    P(SDL_TRUE) P(SDL_FALSE) P(AUDIO_S16SYS)
    P(sizeof(SDL_bool)) P(sizeof(SDL_Rect)) P(sizeof(SDL_AudioFormat)) P(sizeof(SDL_AudioDeviceID))
    P(sizeof(SDL_JoystickID)) P(sizeof(SDL_Keysym)) P(sizeof(SDL_version)) P(offsetof(SDL_version, patch))
    P(sizeof(SDL_RendererInfo)) P(offsetof(SDL_RendererInfo, flags)) P(offsetof(SDL_RendererInfo, max_texture_height))
    P(sizeof(SDL_AudioSpec)) P(offsetof(SDL_AudioSpec, freq)) P(offsetof(SDL_AudioSpec, format))
    P(offsetof(SDL_AudioSpec, channels)) P(offsetof(SDL_AudioSpec, silence)) P(offsetof(SDL_AudioSpec, samples))
    P(offsetof(SDL_AudioSpec, size)) P(offsetof(SDL_AudioSpec, callback)) P(offsetof(SDL_AudioSpec, userdata))
    P(sizeof(SDL_Event)) P(_Alignof(SDL_Event)) P(offsetof(SDL_Event, type))
    P(offsetof(SDL_Event, window.event)) P(offsetof(SDL_Event, window.data1)) P(offsetof(SDL_Event, window.data2))
    P(offsetof(SDL_Event, key.state)) P(offsetof(SDL_Event, key.repeat)) P(offsetof(SDL_Event, key.keysym.scancode))
    P(offsetof(SDL_Event, key.keysym.sym)) P(offsetof(SDL_Event, key.keysym.mod))
    P(offsetof(SDL_Event, cdevice.which)) P(offsetof(SDL_Event, cbutton.which))
    P(offsetof(SDL_Event, cbutton.button)) P(offsetof(SDL_Event, cbutton.state))
    P(offsetof(SDL_Event, adevice.which)) P(offsetof(SDL_Event, adevice.iscapture))
    return 0;
}
