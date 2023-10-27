#ifndef __K_FS_PIPE_H
#define __K_FS_PIPE_H

#include "fs/vfs.h"

#define PIPE_SEL_QUEUE_R 0
#define PIPE_SEL_QUEUE_W 1
#define PIPE_SEL_QUEUE_E 2

typedef struct pipe{
    uint8_t* buffer;
    uint32_t size;
    uint32_t write_ptr;
    uint32_t read_ptr;
	list_t*  wait_queue;
	list_t*  sel_queues[3];
} pipe_t;

fs_node_t* k_fs_pipe_create(uint32_t size);

#endif
