#include "fs/vfs.h"
#include "mem/heap.h"
#include <fs/pipe.h>
#include <string.h>

static uint32_t __k_fs_pipe_read_available(pipe_t * pipe) {
	if (pipe->read_ptr == pipe->write_ptr) {
		return 0;
	}
	if (pipe->read_ptr > pipe->write_ptr) {
		return (pipe->size - pipe->read_ptr) + pipe->write_ptr;
	} else {
		return (pipe->write_ptr - pipe->read_ptr);
	}
}

static uint32_t __k_fs_pipe_write_available(pipe_t* pipe){
    if (pipe->read_ptr == pipe->write_ptr) {
		return pipe->size - 1;
	}

	if (pipe->read_ptr > pipe->write_ptr) {
		return pipe->read_ptr - pipe->write_ptr - 1;
	} else {
		return (pipe->size - pipe->write_ptr) + pipe->read_ptr - 1;
	}
}

static void __k_fs_pipe_bump_write_ptr(pipe_t* pipe){
    pipe->write_ptr++;
    if(pipe->write_ptr == pipe->size){
        pipe->write_ptr = 0;
    }
}

static void __k_fs_pipe_bump_read_ptr(pipe_t* pipe){
    pipe->read_ptr++;
    if(pipe->read_ptr == pipe->size){
        pipe->read_ptr = 0;
    }
}


static uint32_t  __k_fs_pipe_write(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer){
    pipe_t* dev = node->device;

    for(uint32_t i = 0; i < size; i++){
        while(!__k_fs_pipe_write_available(dev));
        dev->buffer[i] = buffer[i];
        __k_fs_pipe_bump_write_ptr(dev);
    }

    return size;
}

static uint32_t  __k_fs_pipe_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer){
    pipe_t* dev = node->device;

    for(uint32_t i = 0; i < size; i++){
        while(!__k_fs_pipe_read_available(dev));
        buffer[i] = dev->buffer[i];
        __k_fs_pipe_bump_read_ptr(dev);
    }

    return size;
}


fs_node_t* k_fs_pipe_create(uint32_t size){
    fs_node_t* node = k_fs_vfs_create_node("[pipe]");

    pipe_t* pipe     = k_calloc(1, sizeof(pipe_t));
    pipe->size       = size;
    pipe->write_ptr  = 0;
    pipe->read_ptr   = 0;
    pipe->buffer     = k_calloc(1, size);

    node->size       = size;
    node->device     = pipe;

    node->fs.write   = __k_fs_pipe_write;
    node->fs.read    = __k_fs_pipe_read;

    return node;
}