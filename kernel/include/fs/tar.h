#ifndef __K_FS_TAR_H
#define __K_FS_TAR_H

#include "kernel.h"

struct tar_header
{
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

typedef struct tar_header tar_header_t;

K_STATUS k_fs_tar_init();

#endif