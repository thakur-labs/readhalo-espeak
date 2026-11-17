// SPDX-License-Identifier: GPL-3.0-or-later
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

struct embedded_file {
    const char *path;
    const unsigned char *bytes;
    size_t size;
};
#include "english.h"

static const struct embedded_file *find_file(const char *path)
{
    for (size_t i = 0; i < sizeof files / sizeof *files; ++i)
        if (strcmp(path, files[i].path) == 0)
            return &files[i];
    return NULL;
}

// eSpeak's data readers use stdio. fmemopen supplies a read-only FILE over
// static bytes and has no file descriptor or dependence on a host filesystem.
FILE *__wrap_fopen(const char *path, const char *mode)
{
    const struct embedded_file *file = find_file(path);
    if (!file || (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0)) {
        errno = ENOENT;
        return NULL;
    }
    return fmemopen((void *)file->bytes, file->size, "r");
}

int __wrap_stat(const char *path, struct stat *out)
{
    memset(out, 0, sizeof *out);
    if (strcmp(path, "/data") == 0) {
        out->st_mode = S_IFDIR;
        return 0;
    }
    const struct embedded_file *file = find_file(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }
    out->st_mode = S_IFREG;
    out->st_size = file->size;
    return 0;
}

// The bridge selects embedded voices by identifier. No discovery is needed.
DIR *__wrap_opendir(const char *path)
{
    (void)path;
    errno = ENOENT;
    return NULL;
}

struct dirent *__wrap_readdir(DIR *dir)
{
    (void)dir;
    return NULL;
}

int __wrap_closedir(DIR *dir)
{
    (void)dir;
    errno = EBADF;
    return -1;
}
