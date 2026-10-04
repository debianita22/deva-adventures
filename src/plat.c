/* Deva's Awesome Adventures - the few things files do differently on Windows and macOS (1.2).
 * SPDX-License-Identifier: MIT
 */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE /* (F_FULLFSYNC, hidden by _POSIX_C_SOURCE) */
#endif
#include "plat.h"

#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static int wide(const char *s, wchar_t *w, int n)
{
    return MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) > 0;
}

int plat_replace(const char *from, const char *to)
{
    wchar_t wf[1024], wt[1024];
    if (!wide(from, wf, 1024) || !wide(to, wt, 1024)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (MoveFileExW(wf, wt, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return 0;
    errno = GetLastError() == ERROR_FILE_NOT_FOUND ? ENOENT : EACCES;
    return -1;
}

int plat_flush_file(const char *path)
{
    wchar_t w[1024];
    if (!wide(path, w, 1024)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    /* (flushing needs a handle that may write) */
    HANDLE h = CreateFileW(w, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        errno = GetLastError() == ERROR_FILE_NOT_FOUND ? ENOENT : EACCES;
        return -1;
    }
    BOOL ok = FlushFileBuffers(h);
    CloseHandle(h);
    if (ok)
        return 0;
    errno = EIO;
    return -1;
}

void plat_flush_dir(const char *dir)
{
    (void)dir; /* (NTFS keeps its folders in its journal) */
}

#else
#include <fcntl.h>
#include <unistd.h>

int plat_replace(const char *from, const char *to)
{
    return rename(from, to);
}

int plat_flush_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    int r;
#ifdef F_FULLFSYNC
    r = fcntl(fd, F_FULLFSYNC);
    if (r == -1) /* (a file system without it: the plain flush) */
#endif
        r = fsync(fd);
    int saved = errno;
    close(fd);
    errno = saved;
    return r;
}

void plat_flush_dir(const char *dir)
{
    int fd = open(dir[0] ? dir : "/", O_RDONLY);
    if (fd >= 0) { /* best effort: some file systems do not sync folders */
        fsync(fd);
        close(fd);
    }
}
#endif
