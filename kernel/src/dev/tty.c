#include "dev/tty.h"
#include "dev/vt.h"
#include "dirent.h"
#include "errno.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "proc/process.h"
#include "sys/tty.h"
#include "util/log.h"
#include "util/types/list.h"
#include "util/types/ringbuffer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define PTY_BUFFER_SIZE 4096


static uint32_t __pty  = 0;
static list_t*  __ptys = 0;

static inline void __rb_putchar(ringbuffer_t* rb, char c) {
	ringbuffer_write(rb, 1, (uint8_t*) &c);
}

#define PUTC_IN(c)  __rb_putchar(pty->in_buffer,  c);
#define PUTC_OUT(c) __rb_putchar(pty->out_buffer, c);


static void __k_dev_tty_pty_process_output_char(tty_t* pty, char c) {
	if (!(pty->ts.c_oflag & OPOST)) {
		goto end;
	}

	if(c == '\n'){
		if(pty->ts.c_oflag & ONLCR) {
			PUTC_OUT('\r');
		} else if(pty->ts.c_oflag & OCRNL) {
			c = '\r';
		}
	}

	if(c == '\r') {
		if(pty->ts.c_oflag & ONLRET) {
			return;
		} 
	}

	if(pty->ts.c_oflag & OLCUC && islower(c)) {
		c = isupper(c);
	}

end:
	PUTC_OUT(c);
	k_dev_vt_tty_callback(pty);
}

#define process_output __k_dev_tty_pty_process_output_char

static void __k_dev_tty_erase(tty_t* pty, size_t amount) {
	if(pty->line_length < amount) {
		amount = pty->line_length;
	}	

	while(amount > 0) {
		char ctrl = 0;
		if((pty->ts.c_lflag & ECHOCTL) && iscntrl(pty->line_buffer[pty->line_length - 1])) {
			ctrl = 1;
		}
		pty->line_buffer[pty->line_length - 1] = '\0';
		pty->line_length--;
		amount--;
		if(pty->ts.c_lflag & ECHO) {
			for(int i = 0; i <= ctrl; i++) {
				process_output(pty, '\010');
			}
		}
	}
}

static void __k_dev_tty_line_putc(tty_t* pty, char c) {
	if(pty->line_length < PTY_BUFFER_SIZE) {
		pty->line_buffer[pty->line_length] = c;
		pty->line_length++;
	}
}

static void __k_dev_tty_line_flush(tty_t* pty) {
	size_t i = 0;
	while(pty->line_length > 0) {
		PUTC_IN(pty->line_buffer[i]);
		pty->line_length--;
		i++;
	}
}

static void __k_dev_tty_pty_process_input_char(tty_t* pty, char c) {
	if(pty->flags & TTY_FLAG_NEXTL) {
		pty->flags &= ~TTY_FLAG_NEXTL;
		__k_dev_tty_line_putc(pty, c);
		if (pty->ts.c_lflag & ECHO) {
			if ((pty->ts.c_lflag & ECHOCTL) && iscntrl(c)) {
				process_output(pty,'^');
				process_output(pty,('@'+c) % 128);
			} else {
				process_output(pty,c);
			}
		}
		return;
	}

	if (pty->ts.c_lflag & ISIG) {
		int sig = -1;
		if (c == pty->ts.c_cc[VINTR]) {
			sig = SIGINT;
		} else if (c == pty->ts.c_cc[VQUIT]) {
			sig = SIGQUIT;
		} else if (c == pty->ts.c_cc[VSUSP]) {
			sig = SIGTSTP;
		}

		if (sig != -1) {
			if (pty->ts.c_lflag & ECHOCTL) {
				process_output(pty,'^');
				process_output(pty,('@' + c) % 128);
			}
			if (pty->process > 0) {
				process_t* process = k_proc_process_find_by_pid(pty->process);
				if(process) {
					k_proc_process_send_signal(process, sig);
				}
			}
			return;
		}
	}

	if (pty->ts.c_iflag & ISTRIP) {
		c &= 0x7F;
	}
	
	if ((pty->ts.c_iflag & IGNCR) && c == '\r') {
		return;
	}
	
	if ((pty->ts.c_iflag & INLCR) && c == '\n') {
		c = '\r';
	} else if ((pty->ts.c_iflag & ICRNL) && c == '\r') {
		c = '\n';
	}

	if(pty->ts.c_lflag & ICANON) {
		if (c == pty->ts.c_cc[VLNEXT] && (pty->ts.c_lflag & IEXTEN)) {
			pty->flags |= TTY_FLAG_NEXTL;
			process_output(pty, '^');
			process_output(pty, '\010');
			return;
		}

		if ((pty->ts.c_iflag & IUCLC) && (pty->ts.c_lflag & IEXTEN) && isupper(c)) {
			c = tolower(c);
		}

		if(pty->ts.c_lflag & ECHOE) {
			if(c == pty->ts.c_cc[VERASE]) {
				__k_dev_tty_erase(pty, 1);
				return;
			} else if(c == pty->ts.c_cc[VWERASE]) {
				if(!(pty->ts.c_lflag & IEXTEN)) {
					process_output(pty, '^');
					process_output(pty, ('@' + c) % 128);
					return;
				}
				while(pty->line_length && pty->line_buffer[pty->line_length - 1] != ' ') {
					__k_dev_tty_erase(pty, 1);
				}
				return;
			}
		} else if((pty->ts.c_lflag & ECHOCTL) && (c == pty->ts.c_cc[VERASE] || c == pty->ts.c_cc[VWERASE])) {
				process_output(pty,'^');
				process_output(pty,('@' + c) % 128);
				return;
		}

		if(c == pty->ts.c_cc[VKILL]) {
			if(pty->ts.c_lflag & ECHOK) {
				__k_dev_tty_erase(pty, pty->line_length);
			} else if(pty->ts.c_lflag & ECHOCTL) {
				process_output(pty,'^');
				process_output(pty,('@' + c) % 128);
			}
			return;
		}

		if (c == pty->ts.c_cc[VEOF]) {
			__k_dev_tty_line_flush(pty);
			return;
		}

		__k_dev_tty_line_putc(pty, c);

		if(pty->ts.c_lflag & ECHO) {
			if((pty->ts.c_lflag & ECHOCTL) && iscntrl(c)) {
				process_output(pty,'^');
				process_output(pty,('@' + c) % 128);
			} else {
				process_output(pty,c);
			}
		}

		if((c == '\n' || c == pty->ts.c_cc[VEOL])) {
			if(!(pty->ts.c_lflag & ECHO) && (pty->ts.c_lflag & ECHONL)) {
				process_output(pty,c);
			}
			pty->line_buffer[pty->line_length - 1] = c;
			__k_dev_tty_line_flush(pty);
			return;
		}
	} else {
		if(pty->ts.c_lflag & ECHO) {
			process_output(pty,c);
		}
		PUTC_IN(c); 
	}
}


static uint32_t __k_dev_tty_pty_read_master(tty_t* pty, uint32_t size, uint8_t* buffer) {
	// k_debug("Reading %d bytes from out", size);
	return ringbuffer_read(pty->out_buffer, size, buffer);
}

static uint32_t __k_dev_tty_pty_read_slave(tty_t* pty, uint32_t size, uint8_t* buffer) {
	// k_debug("Reading %d bytes from in", size);
	if(!(pty->ts.c_lflag & ICANON)) {
		if(pty->ts.c_cc[VMIN] == 0){
			if(size < ringbuffer_read_available(pty->in_buffer)) {
				size = ringbuffer_read_available(pty->in_buffer);
			}
		} else {
			if(size < pty->ts.c_cc[VMIN]) {
				size = pty->ts.c_cc[VMIN];
			}
		}
	}
	return ringbuffer_read(pty->in_buffer, size, buffer);
}

static uint32_t __k_dev_tty_pty_write_master(tty_t* pty, uint32_t size, uint8_t* buffer) {
	// k_debug("Writing %d bytes to in", size);
	for(uint32_t i = 0; i < size; i++) {
		__k_dev_tty_pty_process_input_char(pty, *buffer);
		buffer++;
	}

	return size;
}

static uint32_t __k_dev_tty_pty_write_slave(tty_t* pty, uint32_t size, uint8_t* buffer) {
	// k_debug("Writing %d bytes to out", size);
	for(uint32_t i = 0; i < size; i++) {
		__k_dev_tty_pty_process_output_char(pty, *buffer);
		buffer++;
	}
	

	return size;
}

static uint32_t __k_dev_tty_pty_read(tty_t* pty, uint8_t master, uint32_t size, uint8_t* buffer) {
	if(master) {
		return __k_dev_tty_pty_read_master(pty, size, buffer);
	} else {
		return __k_dev_tty_pty_read_slave(pty, size, buffer);
	}
}

static uint32_t __k_dev_tty_pty_write(tty_t* pty, uint8_t master, uint32_t size, uint8_t* buffer) {
	if(master) {
		return __k_dev_tty_pty_write_master(pty, size, buffer);
	} else {
		return __k_dev_tty_pty_write_slave(pty, size, buffer);
	}
}

static uint8_t __k_dev_tty_pty_is_master(fs_node_t* node) {
	tty_t* pty = node->device;
	return pty->master == node;
}

static uint32_t __k_dev_tty_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	return __k_dev_tty_pty_read(node->device, __k_dev_tty_pty_is_master(node), size, buffer);
}

static uint32_t __k_dev_tty_write(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	return __k_dev_tty_pty_write(node->device, __k_dev_tty_pty_is_master(node), size, buffer);
}

static int __k_dev_tty_ioctl(fs_node_t* node, uint32_t req, void* args) {
	tty_t* pty = node->device;
	if(!pty) {
		return -ENODEV;
	}

	switch(req) {
		case TCGETS:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			memcpy(args, &pty->ts, sizeof(struct termios));
			return 0;
		case TCSETSW:
		case TCSETSF:
		case TCSETS:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			if((pty->ts.c_lflag & ICANON) && !(((struct termios*)args)->c_lflag & ICANON)) {
				__k_dev_tty_line_flush(pty);
			}
			memcpy(&pty->ts, args, sizeof(struct termios));
			return 0;
		case TCIOGPGRP:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			*((pid_t*)args) = pty->process;
			return 0;
		case TCIOSPGRP:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			pty->process = *((pid_t*)args);
			return 0;
		default:
			return -EINVAL;
	}
}

static fs_node_t* __k_dev_tty_create_pty_generic(char* name, tty_t* pty) {
	fs_node_t* node = k_fs_vfs_create_node(name);
	node->device   = pty;
	node->fs.read  = &__k_dev_tty_read;
	node->fs.write = &__k_dev_tty_write;
	node->fs.ioctl = &__k_dev_tty_ioctl;
	node->mode     = O_RDWR;

	return node;
}


static fs_node_t* __k_dev_tty_create_pty_master(tty_t* pty) {
	char name[32];
	snprintf(name, 32, "pty%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static fs_node_t* __k_dev_tty_create_pty_slave(tty_t* pty) {
	char name[32];
	snprintf(name, 32, "pty%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static void __k_dev_tty_sane_ts(tty_t* pty) {
	pty->ts.c_iflag = ICRNL | BRKINT;
	pty->ts.c_oflag = ONLCR | OPOST;
	pty->ts.c_lflag = ECHO | ECHOE | ECHOK | ICANON | ISIG | IEXTEN;
	pty->ts.c_cflag = CREAD | CS8;
	pty->ts.c_cc[VEOF]    =  4;   // ^D 
	pty->ts.c_cc[VEOL]    =  0;   // 
	pty->ts.c_cc[VERASE]  = 0x7f; // ^? 
	pty->ts.c_cc[VINTR]   =  3;   // ^C 
	pty->ts.c_cc[VKILL]   = 21;   // ^U 
	pty->ts.c_cc[VMIN]    =  1;
	pty->ts.c_cc[VQUIT]   = 28;   // ( ^\ ) 
	pty->ts.c_cc[VSTART]  = 17;   // ^Q 
	pty->ts.c_cc[VSTOP]   = 19;   // ^S 
	pty->ts.c_cc[VSUSP]   = 26;   // ^Z 
	pty->ts.c_cc[VTIME]   =  0;
	pty->ts.c_cc[VLNEXT]  = 22;   // ^V 
	pty->ts.c_cc[VWERASE] = 23;   // ^W 

	pty->line_buffer  = k_malloc(PTY_BUFFER_SIZE + 2);
	pty->line_length  = 0;
}

static tty_t* __k_dev_tty_create_pty(uint32_t id, struct winsize* ws) {
	tty_t* pty = k_calloc(1, sizeof(tty_t));
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

	__k_dev_tty_sane_ts(pty);

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
				tty_t* pty = __ptys->data[i];
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
			tty_t* pty = __ptys->data[i];
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

static fs_node_t* __k_dev_tty_create_tty_master(tty_t* pty) {
	char name[32];
	snprintf(name, 32, "tty%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static fs_node_t* __k_dev_tty_create_tty_slave(tty_t* pty) {
	char name[32];
	snprintf(name, 32, "tty%d", pty->id);
	return __k_dev_tty_create_pty_generic(name, pty);
}

static fs_node_t* __k_dev_tty_create_tty(uint32_t id) {
	tty_t* pty = k_calloc(1, sizeof(tty_t));
	pty->id         = id;
	pty->in_buffer  = ringbuffer_create(PTY_BUFFER_SIZE);
	pty->out_buffer = ringbuffer_create(PTY_BUFFER_SIZE);
	pty->master     = __k_dev_tty_create_tty_master(pty);
	pty->slave      = __k_dev_tty_create_tty_slave(pty);
	__k_dev_tty_sane_ts(pty);
	return pty->slave;
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
	tty_t* pty = __k_dev_tty_create_pty(++__pty, ws);

	list_push_back(__ptys, pty);

	*master = pty->master;
	*slave  = pty->slave;

	return 0;
}
