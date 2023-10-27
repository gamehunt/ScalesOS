#include "errno.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "stdio.h"
#include "util/types/list.h"
#include <fs/pipe.h>
#include <string.h>

static int __k_fs_pipe_event2index(uint8_t event) {
	switch(event) {
		case VFS_EVENT_READ:
			return PIPE_SEL_QUEUE_R;
		case VFS_EVENT_WRITE:
			return PIPE_SEL_QUEUE_W;
		case VFS_EVENT_EXCEPT:
			return PIPE_SEL_QUEUE_E;
		default:
			return -EINVAL;
	}
}

static void __k_fs_pipe_notify(fs_node_t* node, uint8_t event) {
	pipe_t* p = node->device;
	int idx = __k_fs_pipe_event2index(event);
	if(idx < 0) {
		return;
	}
	k_proc_process_wakeup_queue_select(p->sel_queues[idx], node, event);
}

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
	if(!dev) {
		return -ENOENT;
	}

	uint32_t written = 0;

    for(written = 0; written < size; written++){
		uint8_t b = 0;
		while(!__k_fs_pipe_write_available(dev)){
			if(node->mode & O_NOBLOCK) {
				b = 1;
				break;
			}
			if(k_proc_process_sleep_on_queue(k_proc_process_current(), dev->wait_queue)) {
				return -ERESTARTSYS;
			}
		}
		if(b) {
			break;
		}
        dev->buffer[dev->write_ptr] = buffer[written];
        __k_fs_pipe_bump_write_ptr(dev);
    }

	k_proc_process_wakeup_queue(dev->wait_queue);
	__k_fs_pipe_notify(node, VFS_EVENT_READ);

    return written;
}

static int32_t __k_fs_pipe_remove(fs_node_t* node) {
    pipe_t* dev = node->device;
	if(!dev) {
		return 0;
	}

	list_free(dev->wait_queue);
	for(int i = 0; i < 3; i++) {
		list_free(dev->sel_queues[i]);
	}
	k_free(dev->buffer);
	k_free(dev);
	return 0;
}

static uint32_t  __k_fs_pipe_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer){
    pipe_t* dev = node->device;
	if(!dev) {
		return -ENOENT;
	}

	uint32_t read = 0;

    for(read = 0; read < size; read++){
		uint8_t b = 0;
		while(!__k_fs_pipe_read_available(dev)) {
			if(node->mode & O_NOBLOCK) {
				b = 1;
				break;
			}
			if(k_proc_process_sleep_on_queue(k_proc_process_current(), dev->wait_queue)) {
				return -ERESTARTSYS;
			}
		}
		if(b) {
			break;
		}
        buffer[read] = dev->buffer[dev->read_ptr];
        __k_fs_pipe_bump_read_ptr(dev);
    }

	k_proc_process_wakeup_queue(dev->wait_queue);
	__k_fs_pipe_notify(node, VFS_EVENT_WRITE);

    return size;
}

static uint8_t __k_fs_pipe_check(fs_node_t* node, uint8_t event) {
	pipe_t* pipe = node->device;
	if(!pipe) {
		return 0;
	}

	switch(event) {
		case VFS_EVENT_READ:
			return __k_fs_pipe_read_available(pipe) > 0;
		case VFS_EVENT_WRITE:
			return __k_fs_pipe_write_available(pipe) > 0;
		default:
			return 0;
	}
}

static int __k_fs_pipe_wait(fs_node_t* node, uint8_t event, process_t* prc) {
	pipe_t* pipe = node->device;
	if(!pipe) {
		return -ENOENT;
	}

	int idx = __k_fs_pipe_event2index(event);
	if(idx < 0) {
		return idx;
	}

	prc->block_node->data.interrupted = 0;

	if(!list_contains(pipe->sel_queues[idx], prc->block_node)) {
		list_push_back(pipe->sel_queues[idx], prc->block_node);
	}

	return 0;
}


fs_node_t* k_fs_pipe_create(uint32_t size){
    fs_node_t* node = k_fs_vfs_create_node("[pipe]");

    pipe_t* pipe     = k_calloc(1, sizeof(pipe_t));
    pipe->size       = size;
    pipe->write_ptr  = 0;
    pipe->read_ptr   = 0;
    pipe->buffer     = k_calloc(1, size);
	pipe->wait_queue = list_create();

	for(int i = 0; i < 3; i++) {
		pipe->sel_queues[i] = list_create();
	}

    node->size       = size;
    node->device     = pipe;

    node->fs.write   = __k_fs_pipe_write;
    node->fs.read    = __k_fs_pipe_read;
	node->fs.rm      = __k_fs_pipe_remove;
	node->fs.check   = __k_fs_pipe_check;
	node->fs.wait    = __k_fs_pipe_wait;

	node->mode       = O_RDWR;
	node->links      = 1;

    return node;
}
