#ifndef __K_DEV_TTY_H
#define __K_DEV_TTY_H

#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "sys/tty.h"
#include "util/types/ringbuffer.h"
#include <stddef.h>

#define TTY_FLAG_NEXTL (1 << 0)

typedef struct {
	uint32_t id;
	
	fs_node_t* master;
	fs_node_t* slave;

	struct winsize ws;
	struct termios ts;

	ringbuffer_t* in_buffer;
	ringbuffer_t* out_buffer;

	char*  line_buffer;
	size_t line_length;

	uint8_t flags;

	pid_t  process;
} tty_t;

K_STATUS k_dev_tty_init();
int      k_dev_tty_create_pty(struct winsize* ws, fs_node_t** master, fs_node_t** slave);

#endif
