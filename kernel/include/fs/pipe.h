#ifndef __K_FS_PIPE_H
#define __K_FS_PIPE_H

#include "fs/vfs.h"

typedef struct pipe{
    uint8_t* buffer;
    uint32_t size;
    uint32_t write_ptr;
    uint32_t read_ptr;
} pipe_t;

fs_node_t* k_fs_pipe_create(uint32_t size);

#endif