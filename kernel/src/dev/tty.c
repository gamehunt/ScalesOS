#include "dev/tty.h"
#include "dirent.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "mem/heap.h"
#include "util/types/list.h"
#include "util/types/ringbuffer.h"
#include <stdio.h>
#include <string.h>

#define PTY_BUFFER_SIZE 4096

typedef struct {
	uint32_t id;
	
	fs_node_t* master;
	fs_node_t* slave;

	struct winsize ws;
	struct termios ts;

	ringbuffer_t* in_buffer;
	ringbuffer_t* out_buffer;
} pty_t;

static uint32_t __pty  = 0;
static list_t*  __ptys = 0;

#define PUTC_IN(c)  ringbuffer_write(pty->in_buffer, 1, (uint8_t*) &c)
#define PUTC_OUT(c) ringbuffer_write(pty->out_buffer, 1, (uint8_t*) &c)

static void __k_dev_tty_pty_process_input_char(pty_t* pty, char c) {
	PUTC_IN(c); //TODO
}

static void __k_dev_tty_pty_process_output_char(pty_t* pty, char c) {
	PUTC_OUT(c); //TODO
}

static uint32_t __k_dev_tty_pty_read_master(pty_t* pty, uint32_t size, uint8_t* buffer) {
	return ringbuffer_read(pty->out_buffer, size, buffer);
}

static uint32_t __k_dev_tty_pty_read_slave(pty_t* pty, uint32_t size, uint8_t* buffer) {
	return ringbuffer_read(pty->in_buffer, size, buffer);
}

static uint32_t __k_dev_tty_pty_write_master(pty_t* pty, uint32_t size, uint8_t* buffer) {
	for(uint32_t i = 0; i < size; i++) {
		__k_dev_tty_pty_process_input_char(pty, *buffer);
	}
	return size;
}

static uint32_t __k_dev_tty_pty_write_slave(pty_t* pty, uint32_t size, uint8_t* buffer) {
	for(uint32_t i = 0; i < size; i++) {
		__k_dev_tty_pty_process_output_char(pty, *buffer);
	}
	return size;
}

static uint32_t __k_dev_tty_pty_read(pty_t* pty, uint8_t master, uint32_t size, uint8_t* buffer) {
	if(master) {
		return __k_dev_tty_pty_read_master(pty, size, buffer);
	} else {
		return __k_dev_tty_pty_read_slave(pty, size, buffer);
	}
}

static uint32_t __k_dev_tty_pty_write(pty_t* pty, uint8_t master, uint32_t size, uint8_t* buffer) {
	if(master) {
		return __k_dev_tty_pty_write_master(pty, size, buffer);
	} else {
		return __k_dev_tty_pty_write_slave(pty, size, buffer);
	}
}

static uint8_t __k_dev_tty_pty_is_master(fs_node_t* node) {
	pty_t* pty = node->device;
	return pty->master == node;
}

static uint32_t __k_dev_tty_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	return __k_dev_tty_pty_read(node->device, __k_dev_tty_pty_is_master(node), size, buffer);
}

static uint32_t __k_dev_tty_write(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	return __k_dev_tty_pty_write(node->device, __k_dev_tty_pty_is_master(node), size, buffer);
}

static fs_node_t* __k_dev_tty_create_pty_generic(char* name, pty_t* pty) {
	fs_node_t* node = k_fs_vfs_create_node(name);
	node->device = pty;
	node->fs.read = &__k_dev_tty_read;
	node->fs.write = &__k_dev_tty_write;
	return node;
}


static fs_node_t* __k_dev_tty_create_pty_master(pty_t* pty) {
	char name[32];
	sprintf(name, "pty%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static fs_node_t* __k_dev_tty_create_pty_slave(pty_t* pty) {
	char name[32];
	sprintf(name, "ptyS%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static pty_t* __k_dev_tty_create_pty(uint32_t id, struct winsize* ws) {
	pty_t* pty = k_calloc(1, sizeof(pty_t));
	pty->id        = id;
	pty->in_buffer  = ringbuffer_create(PTY_BUFFER_SIZE);
	pty->out_buffer = ringbuffer_create(PTY_BUFFER_SIZE);
	pty->master = __k_dev_tty_create_pty_master(pty);
	pty->slave  = __k_dev_tty_create_pty_slave(pty);

	if(ws) {
		memcpy(&pty->ws, ws, sizeof(struct winsize));
	} else {
		pty->ws.ws_rows = 25;
		pty->ws.ws_cols = 80;
	}

	return pty;
}

static struct dirent* __k_dev_tty_pty_root_readdir(fs_node_t* root UNUSED, uint32_t index) {
	struct dirent* dent = k_malloc(sizeof(struct dirent));

	if(index == 0) {
		strcpy(dent->name, ".");
		dent->ino = 0;
		return dent;
	}

	if(index == 1) {
		strcpy(dent->name, "..");
		dent->ino = 1;
		return dent;
	}

	index -= 2;

	for(uint32_t i = 0; i < __ptys->size; i++) {
		if(__ptys->data[i]) {
			if(!index) {
				pty_t* pty = __ptys->data[i];
				sprintf(dent->name, "%d", pty->id);
				dent->ino = pty->id;
				return dent;
			} else {
				index--;
			}
		}
	}

	k_free(dent);
	return 0;
}

static fs_node_t* __k_dev_tty_pty_root_finddir(fs_node_t* root UNUSED, const char* path) {
	uint32_t id = 0;
	for(uint32_t i = 0 ; i < strlen(path); i++) {
		if(path[i] > '9' || path[i] < '0') {
			return NULL;
		}
		id = id * 10 + path[i] - '0';
	}
	for(uint32_t i = 0; i < __ptys->size; i++) {
		if(__ptys->data[i]) {
			pty_t* pty = __ptys->data[i];
			if(pty->id == id) {
				return pty->slave;
			}
		}
	}
	return NULL;
}

static fs_node_t* __k_dev_tty_create_pty_root() {
	fs_node_t* node = k_fs_vfs_create_node("pts");
	node->flags = VFS_DIR;
	node->fs.readdir = &__k_dev_tty_pty_root_readdir;
	node->fs.finddir = &__k_dev_tty_pty_root_finddir;
	return node;
}

static fs_node_t* __k_dev_tty_create_tty(uint32_t id) {
	// TODO
	return k_fs_vfs_create_node("_tty");
}

static fs_node_t* __k_dev_tty_create_tty_link() {
	// TODO
	return k_fs_vfs_create_node("tty");
}

K_STATUS k_dev_tty_init() {
	__ptys = list_create();
	k_fs_vfs_mount_node("/dev/pts", __k_dev_tty_create_pty_root());
	k_fs_vfs_mount_node("/dev/tty", __k_dev_tty_create_tty_link());
	for(int i = 0; i < 8; i++) {
		char path[32];
		snprintf(path, 32, "/dev/tty%d", i);
		k_fs_vfs_mount_node(path, __k_dev_tty_create_tty(i));
	}
	return K_STATUS_OK;
}

int k_dev_tty_create_pty(struct winsize* ws, fs_node_t** master, fs_node_t** slave) {
	pty_t* pty = __k_dev_tty_create_pty(++__pty, ws);

	list_push_back(__ptys, pty);

	*master = pty->master;
	*slave  = pty->slave;

	return 0;
}
