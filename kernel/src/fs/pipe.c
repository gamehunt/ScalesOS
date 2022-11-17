#include "fs/vfs.h"
#include "mem/heap.h"
#include <fs/pipe.h>
#include <string.h>

static void __k_fs_pipe_bump_in_ptr(pipe_t* pipe){
    pipe->in_pointer++;
    if(pipe->in_pointer == pipe->size){
        pipe->in_pointer = 0;
    }
}

static uint32_t  __k_fs_pipe_write(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer){
    pipe_t* dev = node->device;

    //TODO

    return size;
}

static uint32_t  __k_fs_pipe_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    pipe_t* dev = node->device;

    //TODO

    return size;
}


fs_node_t* k_fs_pipe_create(uint32_t size){
    fs_node_t* node = k_fs_vfs_create_node("[pipe]");

    pipe_t* pipe  = k_calloc(1, sizeof(pipe_t));
    pipe->size    = size;
    pipe->in_pointer  = 0;
    pipe->out_pointer = 0;
    pipe->buffer  = k_calloc(1, size);

    node->size   = size;
    node->device = pipe;

    node->fs.write = __k_fs_pipe_write;
    node->fs.read  = __k_fs_pipe_read;

    return node;
}