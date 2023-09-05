#include "dev/vt.h"
#include "dev/console.h"
#include "dev/fb.h"
#include "dev/tty.h"
#include "fs/vfs.h"
#include "proc/process.h"
#include "stdio.h"
#include "util/log.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>

static fs_node_t* __tty0 = 0;

extern void _libk_set_print_callback(void (*pk)(char* a, uint32_t size));

void k_dev_vt_write(char* buff, uint32_t size) {
	if(!__tty0) {
		return;
	}
	k_fs_vfs_write(__tty0, 0, size, buff);
}

static char __k_dev_vt_getc(tty_t* tty) {
	char c;
	int32_t read = k_fs_vfs_read(tty->master, 0, 1, &c);
	if(read != 1) {
		return 0;
	}
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
	if(!__tty0) {
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

		tty_t* tty = __tty0->device;
		k_fs_vfs_write(tty->master, 0, 1, &scancode);
	}
}

static void __k_dev_vt_tasklet() {
	tty_t* tty = __tty0->device;
	while(1) {
		char c = __k_dev_vt_getc(tty);	
		if(!c) {
			continue;
		}
		k_dev_fb_putchar(c, 0xFFFFFFFF, 0x00000000);
	}
}

int k_dev_vt_init() {
	__tty0 = k_fs_vfs_open("/dev/tty0", O_RDWR);
	if(!__tty0) {
		k_err("Failed to open tty0.");
		return 1;
	}
	k_dev_fb_clear(0);
    _libk_set_print_callback(k_dev_vt_write);
    k_dev_console_set_source(k_dev_vt_write);
	k_proc_process_create_tasklet("[vt_routine]", (uintptr_t) &__k_dev_vt_tasklet, NULL);
	return 0;
}
