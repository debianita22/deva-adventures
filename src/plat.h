/* Deva's Awesome Adventures - the few things files do differently on Windows and macOS (1.2).
 * SPDX-License-Identifier: MIT
 *
 * Paths are UTF-8 everywhere (on Windows the PC program declares the UTF-8 code page in its
 * manifest, so the C library's fopen, stat and opendir take them as they are).
 */
#ifndef DEVA_PLAT_H
#define DEVA_PLAT_H

/* rename(from, to) that replaces "to" when it is there, as POSIX rename does (Windows: MoveFileEx) */
int plat_replace(const char *from, const char *to);

/* the file's data down to the disk: fsync (macOS: F_FULLFSYNC, the only flush its disks honour;
   Windows: FlushFileBuffers). 0, or -1 with errno: EINVAL or ENOTSUP = a file system that cannot
   flush, anything else = the file could not be opened or flushed */
int plat_flush_file(const char *path);

/* the folder's entries down to the disk, where a folder can be flushed (best effort) */
void plat_flush_dir(const char *dir);

#endif
