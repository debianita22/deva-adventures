/* Deva's Awesome Adventures - what the PC program asks of the system: Linux, macOS, Windows (1.2).
 * SPDX-License-Identifier: MIT
 */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE /* (realpath and friends, hidden by _POSIX_C_SOURCE) */
#endif
#include "os.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ================================================================== Windows */
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

static bool wide(const char *s, wchar_t *w, int n)
{
    return MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) > 0;
}

static bool utf8(const wchar_t *w, char *s, int n)
{
    return WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL) > 0;
}

static bool std_handle_ok(DWORD which)
{
    HANDLE h = GetStdHandle(which);
    return h && h != INVALID_HANDLE_VALUE;
}

void os_init(void)
{
    /* a GUI program has no console of its own: started from cmd or PowerShell it borrows theirs for
       --help, --version, --check and the messages; with its output already redirected (a test, a
       pipe) it keeps that */
    if (std_handle_ok(STD_OUTPUT_HANDLE) || std_handle_ok(STD_ERROR_HANDLE))
        return;
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (!freopen("CONOUT$", "w", stdout) || !freopen("CONOUT$", "w", stderr))
            return;
    }
}

bool os_exe_dir(char *out, size_t n)
{
    wchar_t w[PATH_MAX];
    DWORD k = GetModuleFileNameW(NULL, w, PATH_MAX);
    if (!k || k >= PATH_MAX || !utf8(w, out, (int)n))
        return false;
    char *sep = strrchr(out, '\\');
    if (sep)
        *sep = 0;
    return true;
}

void *os_lib_open(const char *path)
{
    wchar_t w[PATH_MAX];
    if (!wide(path, w, PATH_MAX))
        return NULL;
    /* a full path: its own folder first for what the library needs in turn */
    bool full = strchr(path, '\\') || strchr(path, '/');
    return (void *)(full ? LoadLibraryExW(w, NULL, LOAD_WITH_ALTERED_SEARCH_PATH) : LoadLibraryW(w));
}

void *os_lib_sym(void *lib, const char *name)
{
    FARPROC f = GetProcAddress((HMODULE)lib, name);
    void *p;
    memcpy(&p, &f, sizeof(p));
    return p;
}

void os_lib_error(char *out, size_t n)
{
    DWORD code = GetLastError();
    wchar_t w[512];
    char text[1024];
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, w, 512, NULL) &&
        utf8(w, text, sizeof(text))) {
        for (size_t k = strlen(text); k > 0 && (text[k - 1] == '\n' || text[k - 1] == '\r' || text[k - 1] == ' ');)
            text[--k] = 0;
        snprintf(out, n, "%s (errore %lu)", text, (unsigned long)code);
    } else {
        snprintf(out, n, "errore %lu di Windows", (unsigned long)code);
    }
}

bool os_saves_dir(char *out, size_t n, const char *app)
{
    const wchar_t *names[] = {L"APPDATA", L"LOCALAPPDATA", L"USERPROFILE"};
    char base[PATH_MAX];
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        const wchar_t *v = _wgetenv(names[i]);
        if (v && *v && utf8(v, base, sizeof(base))) {
            int k = snprintf(out, n, "%s\\%s", base, app);
            return k > 0 && (size_t)k < n;
        }
    }
    return false;
}

bool os_is_terminal(void)
{
    return _isatty(_fileno(stderr)) != 0;
}

void os_dialog(bool error, const char *title, const char *msg)
{
    wchar_t wt[256], wm[2048];
    if (wide(title, wt, 256) && wide(msg, wm, 2048))
        MessageBoxW(NULL, wm, wt, MB_OK | MB_SETFOREGROUND | (error ? MB_ICONERROR : MB_ICONWARNING));
}

static bool make_one(const char *path)
{
    wchar_t w[PATH_MAX];
    if (!wide(path, w, PATH_MAX))
        return false;
    if (CreateDirectoryW(w, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
        return true;
    errno = EACCES;
    return false;
}

bool os_make_dirs(const char *path)
{
    char p[PATH_MAX];
    int k = snprintf(p, sizeof(p), "%s", path);
    if (k <= 0 || (size_t)k >= sizeof(p)) {
        errno = ENAMETOOLONG;
        return false;
    }
    for (char *s = p + 1; *s; s++) {
        if (*s != '\\' && *s != '/')
            continue;
        if (s[-1] == ':' || s[-1] == '\\' || s[-1] == '/') /* (C:\ and the \\ of a network path) */
            continue;
        char c = *s;
        *s = 0;
        bool ok = make_one(p);
        *s = c;
        if (!ok)
            return false;
    }
    return make_one(p);
}

bool os_writable(const char *dir)
{
    wchar_t w[PATH_MAX];
    return wide(dir, w, PATH_MAX) && _waccess(w, 2) == 0;
}

int os_lock(const char *path)
{
    wchar_t w[PATH_MAX];
    if (!wide(path, w, PATH_MAX))
        return -1;
    /* nobody else may open it while this copy runs; Windows drops it when the program ends */
    HANDLE h = CreateFileW(w, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (h != INVALID_HANDLE_VALUE)
        return 1;
    return GetLastError() == ERROR_SHARING_VIOLATION ? 0 : -1;
}

FILE *os_fopen(const char *path, const char *mode)
{
    wchar_t w[PATH_MAX], m[16];
    if (!wide(path, w, PATH_MAX) || !wide(mode, m, 16))
        return NULL;
    return _wfopen(w, m);
}

void os_default_env(const char *name, const char *value)
{
    const char *v = getenv(name);
    if (!v || !*v)
        _putenv_s(name, value);
}

/* ================================================================== Linux and macOS */
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <spawn.h>
extern char **environ;
#endif

void os_init(void) {}

bool os_exe_dir(char *out, size_t n)
{
    char buf[PATH_MAX];
#if defined(__APPLE__)
    char raw[PATH_MAX];
    uint32_t size = sizeof(raw);
    if (_NSGetExecutablePath(raw, &size) != 0 || !realpath(raw, buf))
        return false;
#else
    ssize_t k = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (k <= 0)
        return false;
    buf[k] = 0;
#endif
    char *slash = strrchr(buf, '/');
    if (slash)
        *(slash == buf ? slash + 1 : slash) = 0;
    int k2 = snprintf(out, n, "%s", buf);
    return k2 > 0 && (size_t)k2 < n;
}

void *os_lib_open(const char *path)
{
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

void *os_lib_sym(void *lib, const char *name)
{
    return dlsym(lib, name);
}

void os_lib_error(char *out, size_t n)
{
    const char *e = dlerror();
    snprintf(out, n, "%s", e ? e : "?");
}

bool os_saves_dir(char *out, size_t n, const char *app)
{
    const char *home = getenv("HOME");
    int k;
#if defined(__APPLE__)
    if (!home || !*home)
        return false;
    k = snprintf(out, n, "%s/Library/Application Support/%s", home, app);
#else
    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg && xdg[0] == '/')
        k = snprintf(out, n, "%s/%s", xdg, app);
    else if (home && *home)
        k = snprintf(out, n, "%s/.local/share/%s", home, app);
    else
        return false;
#endif
    return k > 0 && (size_t)k < n;
}

bool os_is_terminal(void)
{
    return isatty(STDERR_FILENO) != 0;
}

#if defined(__APPLE__)
/* a string for AppleScript: in quotes, with \ and " escaped */
static void apple_string(char *out, size_t n, const char *s)
{
    size_t k = 0;
    if (n < 3)
        return;
    out[k++] = '"';
    for (; *s && k + 3 < n; s++) {
        if (*s == '"' || *s == '\\')
            out[k++] = '\\';
        out[k++] = *s;
    }
    out[k++] = '"';
    out[k] = 0;
}
#endif

void os_dialog(bool error, const char *title, const char *msg)
{
#if defined(__APPLE__)
    char t[512], m[2600], script[3300];
    apple_string(t, sizeof(t), title);
    apple_string(m, sizeof(m), msg);
    snprintf(script, sizeof(script), "display alert %s message %s as %s", t, m, error ? "critical" : "warning");
    char *argv[] = {"osascript", "-e", script, NULL};
    pid_t pid;
    if (posix_spawnp(&pid, "osascript", NULL, NULL, argv, environ) == 0)
        waitpid(pid, NULL, 0);
#else
    (void)error;
    pid_t pid = fork();
    if (pid == 0) {
        char *zenity[] = {"zenity", "--error", "--title", (char *)title, "--text", (char *)msg, NULL};
        char *kdialog[] = {"kdialog", "--title", (char *)title, "--error", (char *)msg, NULL};
        char *xmessage[] = {"xmessage", "-center", (char *)msg, NULL};
        execvp(zenity[0], zenity);
        execvp(kdialog[0], kdialog);
        execvp(xmessage[0], xmessage);
        _exit(127);
    }
    if (pid > 0)
        waitpid(pid, NULL, 0);
#endif
}

bool os_make_dirs(const char *path)
{
    char p[PATH_MAX];
    int k = snprintf(p, sizeof(p), "%s", path);
    if (k <= 0 || (size_t)k >= sizeof(p)) {
        errno = ENAMETOOLONG;
        return false;
    }
    for (char *s = p + 1; *s; s++) {
        if (*s != '/')
            continue;
        *s = 0;
        if (mkdir(p, 0755) != 0 && errno != EEXIST)
            return false;
        *s = '/';
    }
    return mkdir(p, 0755) == 0 || errno == EEXIST;
}

bool os_writable(const char *dir)
{
    return access(dir, W_OK) == 0;
}

int os_lock(const char *path)
{
    int fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0)
        return -1; /* (a folder that takes no lock files) */
    struct flock fl;
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &fl) != 0) {
        close(fd);
        return 0;
    }
    return 1; /* (kept open: the lock goes when the program ends) */
}

FILE *os_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

void os_default_env(const char *name, const char *value)
{
    setenv(name, value, 0);
}
#endif
