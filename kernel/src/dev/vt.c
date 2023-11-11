#include "dev/vt.h"
#include "dev/fb.h"
#include "dev/serial.h"
#include "dev/tty.h"
#include "errno.h"
#include "fs/vfs.h"
#include "mem/paging.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "proc/spinlock.h"
#include "stdio.h"
#include "util/log.h"
#include "util/types/ringbuffer.h"
#include "sys/tty.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static fs_node_t* __tty0  = 0;
static fs_node_t* __atty  = 0;
static fs_node_t* __ttys[TTY_AMOUNT];
static fs_node_t* __vts[TTY_AMOUNT];
static spinlock_t vt_lock = 0;

typedef struct {
	char*    data;
	uint32_t occupied_size;
} vt_buffer_t;

typedef struct {
	struct vt_mode mode;
	char        display_mode;
	fs_node_t*  vcs;
	tty_t*      tty;
	vt_buffer_t buffer;
	uint8_t     next;
} vt_t;

extern void _libk_set_print_callback(void (*pk)(char* a, uint32_t size));

static int32_t __k_dev_vt_finish_switch(vt_t* old_vt) {
	LOCK(vt_lock);

	uint8_t number = old_vt->next - 1;
	old_vt->next = 0;

	fs_node_t* tty = __ttys[number];
	__atty = tty;

	vt_t* vt   = __vts[number]->device;

	if(vt->mode.mode == VT_MODE_AUTO) {
		if(vt->display_mode == VT_DISP_MODE_TEXT) {
			k_dev_fb_clear(0);
			vt_buffer_t buff = vt->buffer;
			k_dev_fb_restore_text(buff.data, buff.occupied_size);
		} else {
			fb_save_data_t* dat = vt->vcs->device;
			k_dev_fb_restore(dat);
		}
	} else {
		process_t* process = k_proc_process_find_by_pid(vt->tty->process);
		if(process) {
			k_proc_process_send_signal(process, vt->mode.acqsig);
		}
	}

	UNLOCK(vt_lock);
	return 0;
}

int32_t k_dev_vt_change(uint8_t number) {
	LOCK(vt_lock);

	if(number >= TTY_AMOUNT) {
		UNLOCK(vt_lock);
		return -EINVAL;
	}

	vt_t* old_vt = NULL;

	if(__atty) {
		tty_t* atty_dev = __atty->device;
		old_vt   = __vts[atty_dev->id]->device;
		if(old_vt->mode.mode == VT_MODE_AUTO) {
			k_dev_fb_save(old_vt->vcs->device);
		} else {
			old_vt->next = number + 1;
			process_t* process = k_proc_process_find_by_pid(old_vt->tty->process);
			if(process) {
				k_proc_process_send_signal(process, old_vt->mode.relsig);
			}
			UNLOCK(vt_lock);
			return 0;
		}
	} else {
		old_vt = __vts[number]->device;
	}

	old_vt->next = number + 1;

	UNLOCK(vt_lock);
	return __k_dev_vt_finish_switch(old_vt);
}

static char __k_dev_vt_getc(tty_t* tty) {
	if(!ringbuffer_read_available(tty->out_buffer)) {
		return 0;
	}
	char c;
	k_fs_vfs_read(tty->master, 0, 1, &c);
	return c;
}

static const char __scancodes[128] = { 
	0,
	27, // ESC
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=',
	'\b',
	'\t',
	'q','w','e','r','t','y','u','i','o','p','[',']','\n',
	0, // LCTRL
	'a','s','d','f','g','h','j','k','l',';','\'', '`',
	0, // LSHIFT
	'\\','z','x','c','v','b','n','m',',','.','/',
	0, // RSHIFT
	'*',
	0, // LALT
	' ', 
	0, // CapsLock
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 - F10
	0, // NUMLOCK
	0, // SCROLLOCK
	0, // EX HOME
	0, // EX UP
	0, // EX PG UP
	'-',
	0, // EX L ARROW
	0,
	0, // EX R ARROW
	'+',
	0, // EX END
	0, // EX DOWN
	0, // EX PG DOWN
	0, // EX INSERT
	0, // EX DELETE
	0, 0, 0,
	0, // F11
	0, // F12 
	0, // Other
};

static const char __scancodes_shifted[128] = {
	0, 27,
	'!','@','#','$','%','^','&','*','(',')',
	'_','+','\b',
	'\t', /* tab */
	'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
	0, /* control */
	'A','S','D','F','G','H','J','K','L',':','"', '~',
	0, /* left shift */
	'|','Z','X','C','V','B','N','M','<','>','?',
	0, /* right shift */
	'*',
	0, /* alt */
	' ', /* space */
	0, /* caps lock */
	0, /* F1 [59] */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, /* ... F10 */
	0, /* 69 num lock */
	0, /* scroll lock */
	0, /* home */
	0, /* up */
	0, /* page up */
	'-',
	0, /* left arrow */
	0,
	0, /* right arrow */
	'+',
	0, /* 79 end */
	0, /* down */
	0, /* page down */
	0, /* insert */
	0, /* delete */
	0, 0, 0,
	0, /* F11 */
	0, /* F12 */
	0, /* everything else */
};


#define MOD_RCTRL  (1 << 0)
#define MOD_LCTRL  (1 << 1)
#define MOD_CTRL   (MOD_LCTRL | MOD_RCTRL)
#define MOD_RSHIFT (1 << 2) 
#define MOD_LSHIFT (1 << 3)
#define MOD_SHIFT  (MOD_LSHIFT | MOD_RSHIFT)
#define MOD_ALT    (1 << 4)
#define MOD_CAPS   (1 << 5)

static uint8_t ex   = 0;
static uint8_t mods = 0;

void k_dev_vt_handle_scancode(uint8_t v) {
	if(!__atty) {
		return;
	}

	if(v == 0xE0) {
		ex = 1;
		return;
	}

	if(ex) {
		// TODO
		ex = 0;
		return;
	}
	
	ex = 0;

	char is_up = v & 0x80;
	v &= 0x7f;

	if(v > sizeof(__scancodes)) {
		return;
	}

	char scancode = __scancodes[v]; 

	if(!scancode) {
		switch(v) {
			case 0x1D:
				if(!is_up) {
					mods |= MOD_LCTRL;
				} else {
					mods &= ~MOD_LCTRL;
				}
				break;
			case 0x2A:
				if(!is_up) {
					mods |= MOD_LSHIFT;
				} else {
					mods &= ~MOD_LSHIFT;
				}
				break;
			case 0x36:
				if(!is_up) {
					mods |= MOD_RSHIFT;
				} else {
					mods &= ~MOD_RSHIFT;
				}
				break;
			case 0x38:
				if(!is_up) {
					mods |= MOD_ALT;
				} else {
					mods &= ~MOD_ALT;
				}
				break;
			case 0x3A:
				if(!is_up) {
					break;
				}
				if(mods & MOD_CAPS) {
					mods &= ~MOD_CAPS;
				} else {
					mods |= MOD_CAPS;
				}
				break;
		}
		if(!is_up && (mods & MOD_ALT) && (mods & MOD_CTRL) && (v >= 0x3B && v <= 0x44)) {
			k_dev_vt_change(v - 0x3B);
		}
		return;
	} else {
		if(is_up) {
			return;
		}

		if(mods & MOD_CAPS) {
			scancode = isupper(scancode) ? tolower(scancode) : toupper(scancode);
		}

		if(mods & MOD_CTRL) {
			scancode = toupper(__scancodes[v]);
			if (scancode == '-') scancode = '_';
			if (scancode == '`') scancode = '@';
			int out = (int)(scancode - 0x40);
			if (out < 0 || out > 0x1F) {
				scancode = __scancodes[v];
			} else {
				scancode = out;
			}
		} else {
			scancode = (mods & MOD_SHIFT) ? __scancodes_shifted[v] : __scancodes[v];
		}

		if(scancode == '\b') {
			scancode = 0x7F;
		}

		tty_t* tty = __atty->device;
		k_fs_vfs_write(tty->master, 0, 1, &scancode);
	}
}

void k_dev_vt_tty_callback(struct tty* tty) {
	LOCK(vt_lock);
	tty_t* a = __atty->device;
	char c = __k_dev_vt_getc(tty);
	while(c) {
		vt_t* c_vt = __vts[a->id]->device;
		vt_t* i_vt = __vts[tty->id]->device;
		if(a->id == tty->id && c_vt->display_mode != VT_DISP_MODE_GRAPHIC) {
			k_dev_fb_putchar(c, 0xFFFFFFFF, 0x00000000);
		}
		if(tty == __tty0->device) {
			k_dev_serial_putchar(c);
		}
		if(i_vt->display_mode != VT_DISP_MODE_GRAPHIC) {
			k_fs_vfs_write(__vts[tty->id], 0, 1, &c);
		}
		c = __k_dev_vt_getc(tty);
	} 
	UNLOCK(vt_lock);
}

static void __k_dev_vt_klog_write(char* buff, uint32_t size) {
	if(!__tty0) {
		return;
	}
	k_fs_vfs_write(__tty0, 0, size, buff);
}

static uint32_t __k_dev_vt_console_readlink(fs_node_t* node UNUSED, uint8_t* buf, uint32_t size) {
	strncpy(buf, "/dev/tty0", size);
	return strlen(buf) + 1;
}

#define CHECK_OWNER(vt) \
	if(vt->mode.mode == VT_MODE_PROCESS && vt->tty->process && vt->tty->process != k_proc_process_current()->pid) { \
		return -EPERM; \
	}

static int __k_dev_vt_ioctl(fs_node_t* node, uint32_t req, void* args) {
	vt_t* vt = node->device;
	switch(req) {
		case VT_ACTIVATE:
			CHECK_OWNER(vt)
			return k_dev_vt_change(node->inode);
		case VT_GETMODE:
			if(!IS_VALID_PTR(args)) {
				return -EINVAL;
			}
			*((struct vt_mode*)args) = vt->mode;
			return 0;
		case VT_SETMODE:
			if(!IS_VALID_PTR(args)) {
				return -EINVAL;
			}
			CHECK_OWNER(vt)
			vt->mode = *((struct vt_mode*)args);
			return 0;
		case VT_RELDISP:
			CHECK_OWNER(vt)
			if(!vt->next) {
				return 0;
			}
			if(((char) args) == 1) {
				__k_dev_vt_finish_switch(vt);
			}
			break;
		case KDGETMODE:
			return vt->display_mode;
		case KDSETMODE:
			CHECK_OWNER(vt)
			vt->display_mode = (char) args;
			return 0;
		default:
			return -EINVAL;
	}
}

static fs_node_t* __k_dev_vt_create_console() {
	fs_node_t* node = k_fs_vfs_create_node("console");
	node->flags = VFS_SYMLINK;
	node->fs.readlink = &__k_dev_vt_console_readlink;
	return node;
}

static uint32_t __k_dev_vt_cvt_readlink(fs_node_t* node UNUSED, uint8_t* buf, uint32_t size) {
	snprintf(buf, size, "/dev/vt%d", ((tty_t*)__atty->device)->id);
	return strlen(buf) + 1;
}

static fs_node_t* __k_dev_vt_create_current_vt() {
	fs_node_t* node = k_fs_vfs_create_node("vt");
	node->flags = VFS_SYMLINK;
	node->fs.readlink = &__k_dev_vt_cvt_readlink;
	return node;
}

static uint32_t __k_dev_vt_cvcs_readlink(fs_node_t* node UNUSED, uint8_t* buf, uint32_t size) {
	snprintf(buf, size, "/dev/vcs%d", ((tty_t*)__atty->device)->id);
	return strlen(buf) + 1;
}

static fs_node_t* __k_dev_vt_create_current_vcs() {
	fs_node_t* node = k_fs_vfs_create_node("vcs");
	node->flags = VFS_SYMLINK;
	node->fs.readlink = &__k_dev_vt_cvcs_readlink;
	return node;
}

static uint32_t __k_dev_vt_read(fs_node_t* node, uint32_t offs, uint32_t size, uint8_t* buffer) {
	if(!node->device) {
		return 0;
	}

	vt_t* vt = node->device;
	vt_buffer_t* buff = &vt->buffer;

	if(offs >= buff->occupied_size) {
		return 0;
	}

	if(offs + size >= buff->occupied_size) {
		size = buff->occupied_size - offs;
	}

	if(!size) {
		return 0;
	}

	memcpy(buffer, buff->data + offs, size);

	return 0;
}

static uint32_t __k_dev_vt_write(fs_node_t* node, uint32_t offs, uint32_t size, uint8_t* buffer) {
	vt_t* vt = node->device;
	vt_buffer_t* buff = &vt->buffer;

	offs = buff->occupied_size;

	uint32_t free_size = node->size - offs;

	if(free_size <= size) {
		buff->data = k_realloc(buff->data, node->size * 2);
		node->size *= 2;
	}

	memcpy(buff->data + offs, buffer, size);

	buff->occupied_size += size;

	return size;
}

static fs_node_t* __k_dev_vcs_create(uint8_t number) {
	char buffer[5];
	snprintf(buffer, 5, "vcs%d", number);
	fs_node_t* vcs = k_fs_vfs_create_node(buffer);
	vcs->inode = number;
	vcs->flags = VFS_CHARDEV;
	
	fb_save_data_t* b   = k_malloc(sizeof(fb_save_data_t));
	vcs->device = b;

	return vcs;
}

static fs_node_t* __k_dev_vt_create(uint8_t number) {
	char buffer[5];
	snprintf(buffer, 5, "vt%d", number);
	fs_node_t* vt = k_fs_vfs_create_node(buffer);
	vt->inode = number;
	vt->flags = VFS_CHARDEV;
	
	vt_buffer_t* b   = k_malloc(sizeof(vt_buffer_t));
	b->occupied_size = 0;
	b->data          = malloc(32);

	vt_t* vtd   = k_malloc(sizeof(vt_t));
	vtd->display_mode   = VT_DISP_MODE_TEXT;
	vtd->buffer = *b;
	vtd->vcs    = __k_dev_vcs_create(number);
	vtd->mode.mode   = VT_MODE_AUTO;
	vtd->mode.relsig = SIGUSR1;
	vtd->mode.acqsig = SIGUSR1;

	vt->device = vtd;
	vt->size   = 32;

	vt->fs.read  = __k_dev_vt_read;
	vt->fs.write = __k_dev_vt_write;
	vt->fs.ioctl = __k_dev_vt_ioctl;	

	return vt;
}

int k_dev_vt_init() {
	for(int i = 0; i < TTY_AMOUNT; i++) {
		char path[32];

		snprintf(path, 32, "/dev/tty%d", i);
		__ttys[i] = k_fs_vfs_open(path, O_RDWR);

		snprintf(path, 32, "/dev/vt%d", i);
		__vts[i] =  __k_dev_vt_create(i);
		((vt_t*) __vts[i]->device)->tty = __ttys[i]->device;
		k_fs_vfs_mount_node(path, __vts[i]);
		__vts[i] = k_fs_vfs_dup(__vts[i]);
		__vts[i]->mode = O_RDWR;

		snprintf(path, 32, "/dev/vcs%d", i);
		k_fs_vfs_mount_node(path, ((vt_t*)__vts[i]->device)->vcs);

	}
	__tty0 = __ttys[0];
	k_fs_vfs_mount_node("/dev/console", __k_dev_vt_create_console());
	k_fs_vfs_mount_node("/dev/vt", __k_dev_vt_create_current_vt());
	k_fs_vfs_mount_node("/dev/vcs", __k_dev_vt_create_current_vcs());
	k_dev_vt_change(0);
    _libk_set_print_callback(&__k_dev_vt_klog_write);
	return 0;
}
