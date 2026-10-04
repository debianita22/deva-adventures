/* Deva's Awesome Adventures - what the PC program asks of the system: Linux, macOS, Windows (1.2).
 *
 * Paths are UTF-8 strings on every system (on Windows they are turned into wide strings here, and the
 * program's manifest gives the C library the UTF-8 code page for the rest).
 */
#ifndef DEVA_OS_H
#define DEVA_OS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32)
#define OS_NAME "windows"
#define OS_SEP "\\"
#elif defined(__APPLE__)
#define OS_NAME "macos"
#define OS_SEP "/"
#else
#define OS_NAME "linux"
#define OS_SEP "/"
#endif

/* Windows: the console of the terminal it was started from, if any (a GUI program has none) */
void os_init(void);

/* the folder of the program itself (symbolic links resolved) */
bool os_exe_dir(char *out, size_t n);

/* a shared library: open, a function of it, what went wrong (for the grown-ups) */
void *os_lib_open(const char *path);
void *os_lib_sym(void *lib, const char *name);
void os_lib_error(char *out, size_t n);

/* the folder where the saves go when none is given: ~/.local/share/deva-adventures,
   ~/Library/Application Support/deva-adventures, %APPDATA%\deva-adventures */
bool os_saves_dir(char *out, size_t n, const char *app);

/* stderr is a terminal: messages go there, not into a dialog */
bool os_is_terminal(void);

/* a message without SDL: zenity, kdialog or xmessage; osascript; MessageBox */
void os_dialog(bool error, const char *title, const char *msg);

/* the folder and the ones above it; a folder where files can be written */
bool os_make_dirs(const char *path);
bool os_writable(const char *dir);

/* the lock that keeps one copy of the game open: 1 taken (kept until the program ends),
   0 another copy has it, -1 it cannot be taken here (the game goes on) */
int os_lock(const char *path);

/* fopen with a UTF-8 path */
FILE *os_fopen(const char *path, const char *mode);

/* a variable of the environment for the libraries loaded later, unless the user set it */
void os_default_env(const char *name, const char *value);

#endif
