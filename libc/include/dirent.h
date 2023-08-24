#ifndef __DIRENT_H
#define __DIRENT_H

#include <stdint.h>
#include <stdio.h>

struct dirent {
    char name[256];
    uint32_t ino;
};

#if !defined(__LIBK) && !defined(__KERNEL)

int            closedir(DIR *);
DIR           *opendir(const char *);
struct dirent *readdir(DIR *);

int            readdir_r(DIR *restrict, struct dirent *restrict, struct dirent **restrict);

void           rewinddir(DIR *);

void           seekdir(DIR *, long);
long           telldir(DIR *);

#endif

#endif
