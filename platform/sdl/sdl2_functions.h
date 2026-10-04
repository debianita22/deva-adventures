/* Deva's Awesome Adventures - the SDL2 functions the PC program loads at run time (1.1), as
 * X(return type, name, parameters). Kept apart from sdl2_api.h so that
 * tools/harness/check_sdl2_api.sh can hold every one of them against the real SDL2 prototypes. */
#ifndef DEVA_SDL2_FUNCTIONS_H
#define DEVA_SDL2_FUNCTIONS_H

/* the functions, as X(return type, name, parameters); the optional ones may be missing */
#define SDL2_FUNCTIONS(X)                                                                                    \
    X(int, SDL_Init, (Uint32 flags))                                                                         \
    X(void, SDL_Quit, (void))                                                                                \
    X(void, SDL_GetVersion, (SDL_version * ver))                                                             \
    X(const char *, SDL_GetError, (void))                                                                    \
    X(SDL_bool, SDL_SetHint, (const char *name, const char *value))                                          \
    X(SDL_Window *, SDL_CreateWindow, (const char *title, int x, int y, int w, int h, Uint32 flags))         \
    X(void, SDL_DestroyWindow, (SDL_Window * window))                                                        \
    X(int, SDL_SetWindowFullscreen, (SDL_Window * window, Uint32 flags))                                     \
    X(Uint32, SDL_GetWindowFlags, (SDL_Window * window))                                                     \
    X(void, SDL_SetWindowIcon, (SDL_Window * window, SDL_Surface * icon))                                    \
    X(void, SDL_SetWindowMinimumSize, (SDL_Window * window, int min_w, int min_h))                           \
    X(void, SDL_ShowWindow, (SDL_Window * window))                                                           \
    X(int, SDL_GetDisplayUsableBounds, (int display_index, SDL_Rect *rect))                                  \
    X(int, SDL_ShowCursor, (int toggle))                                                                     \
    X(SDL_Renderer *, SDL_CreateRenderer, (SDL_Window * window, int index, Uint32 flags))                    \
    X(void, SDL_DestroyRenderer, (SDL_Renderer * renderer))                                                  \
    X(int, SDL_GetRendererInfo, (SDL_Renderer * renderer, SDL_RendererInfo *info))                           \
    X(const char *, SDL_GetCurrentVideoDriver, (void))                                                       \
    X(const char *, SDL_GetCurrentAudioDriver, (void))                                                       \
    X(int, SDL_RenderSetLogicalSize, (SDL_Renderer * renderer, int w, int h))                                \
    X(int, SDL_RenderSetIntegerScale, (SDL_Renderer * renderer, SDL_bool enable))                            \
    X(int, SDL_SetRenderDrawColor, (SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a))            \
    X(int, SDL_RenderClear, (SDL_Renderer * renderer))                                                       \
    X(int, SDL_RenderCopy, (SDL_Renderer * renderer, SDL_Texture * t, const SDL_Rect *src, const SDL_Rect *dst)) \
    X(void, SDL_RenderPresent, (SDL_Renderer * renderer))                                                    \
    X(SDL_Texture *, SDL_CreateTexture, (SDL_Renderer * renderer, Uint32 format, int access, int w, int h))  \
    X(void, SDL_DestroyTexture, (SDL_Texture * texture))                                                     \
    X(int, SDL_UpdateTexture, (SDL_Texture * texture, const SDL_Rect *rect, const void *pixels, int pitch))   \
    X(SDL_Surface *, SDL_CreateRGBSurfaceWithFormatFrom,                                                     \
      (void *pixels, int w, int h, int depth, int pitch, Uint32 format))                                     \
    X(void, SDL_FreeSurface, (SDL_Surface * surface))                                                        \
    X(int, SDL_PollEvent, (SDL_Event * event))                                                               \
    X(const Uint8 *, SDL_GetKeyboardState, (int *numkeys))                                                   \
    X(SDL_Keymod, SDL_GetModState, (void))                                                                   \
    X(SDL_AudioDeviceID, SDL_OpenAudioDevice,                                                                \
      (const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained,             \
       int allowed_changes))                                                                                 \
    X(void, SDL_CloseAudioDevice, (SDL_AudioDeviceID dev))                                                   \
    X(void, SDL_PauseAudioDevice, (SDL_AudioDeviceID dev, int pause_on))                                     \
    X(int, SDL_QueueAudio, (SDL_AudioDeviceID dev, const void *data, Uint32 len))                            \
    X(Uint32, SDL_GetQueuedAudioSize, (SDL_AudioDeviceID dev))                                               \
    X(void, SDL_ClearQueuedAudio, (SDL_AudioDeviceID dev))                                                   \
    X(int, SDL_NumJoysticks, (void))                                                                         \
    X(SDL_bool, SDL_IsGameController, (int joystick_index))                                                  \
    X(SDL_GameController *, SDL_GameControllerOpen, (int joystick_index))                                    \
    X(void, SDL_GameControllerClose, (SDL_GameController * controller))                                      \
    X(Uint8, SDL_GameControllerGetButton, (SDL_GameController * controller, int button))                     \
    X(SDL_Joystick *, SDL_GameControllerGetJoystick, (SDL_GameController * controller))                      \
    X(SDL_JoystickID, SDL_JoystickInstanceID, (SDL_Joystick * joystick))                                     \
    X(Uint64, SDL_GetPerformanceCounter, (void))                                                             \
    X(Uint64, SDL_GetPerformanceFrequency, (void))                                                           \
    X(void, SDL_Delay, (Uint32 ms))                                                                          \
    X(int, SDL_ShowSimpleMessageBox, (Uint32 flags, const char *title, const char *message, SDL_Window *window))

#define SDL2_OPTIONAL_FUNCTIONS(X)                                                                           \
    X(int, SDL_GameControllerRumble, (SDL_GameController * c, Uint16 low, Uint16 high, Uint32 duration_ms))  \
    X(SDL_GameControllerType, SDL_GameControllerGetType, (SDL_GameController * c))

#endif
