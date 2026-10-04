/* Deva's Awesome Adventures - a gamepad for test_linux.sh, where there is none (1.1).
 *
 * Loaded by the PC program in place of SDL2 (DEVA_SDL2_LIB=fake_pad.so): every function is the real
 * SDL2's (this library depends on it) but two. SDL_Init also plugs in a virtual gamepad (SDL's own
 * virtual joystick: the standard buttons in SDL's order, so 0 is the bottom face button, 1 the right
 * one, 6 START, 11-14 the D-pad), and SDL_PollEvent first carries out the lines appended to the file
 * named by DEVA_FAKE_PAD: "+N" presses button N, "-N" lets it go, "unplug" pulls the gamepad out.
 *
 *   cc -shared -fPIC $(sdl2-config --cflags) -o fake_pad.so tools/harness/fake_pad.c $(sdl2-config --libs) -ldl
 */
#define _GNU_SOURCE
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_index = -1;
static SDL_Joystick *s_pad;
static FILE *s_cmds;

int SDL_Init(Uint32 flags)
{
    static int (*real)(Uint32);
    if (!real)
        *(void **)&real = dlsym(RTLD_NEXT, "SDL_Init");
    int r = real(flags);
    if (r == 0 && (flags & SDL_INIT_GAMECONTROLLER) && s_index < 0) {
        s_index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, SDL_CONTROLLER_BUTTON_MAX, 0);
        if (s_index >= 0)
            s_pad = SDL_JoystickOpen(s_index);
        fprintf(stderr, "fake_pad: virtual gamepad %s\n", s_pad ? "plugged in" : SDL_GetError());
    }
    return r;
}

int SDL_PollEvent(SDL_Event *event)
{
    static int (*real)(SDL_Event *);
    if (!real)
        *(void **)&real = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (!s_cmds && getenv("DEVA_FAKE_PAD"))
        s_cmds = fopen(getenv("DEVA_FAKE_PAD"), "r");
    if (s_cmds && s_pad) {
        char line[64];
        clearerr(s_cmds);
        while (s_pad && fgets(line, sizeof(line), s_cmds)) {
            if (line[0] == '+' || line[0] == '-') {
                SDL_JoystickSetVirtualButton(s_pad, atoi(line + 1), line[0] == '+' ? SDL_PRESSED : SDL_RELEASED);
            } else if (!strncmp(line, "unplug", 6)) {
                SDL_JoystickClose(s_pad);
                s_pad = NULL;
                SDL_JoystickDetachVirtual(s_index);
            }
        }
    }
    return real(event);
}
