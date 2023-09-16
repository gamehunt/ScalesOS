#include <dev/fb.h>
#include <mem/heap.h>
#include <string.h>

#include "fs/vfs.h"
#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "multiboot.h"
#include "util/log.h"

static fb_pos_t   pos = {0, 0};
static fb_impl_t* impl = 0;
static fb_info_t  info;

void k_dev_fb_write(char* buff, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        k_dev_fb_putchar(buff[i], 0xFFFFFFFF, 0x0);
    }
}

static fs_node_t* __k_dev_fb_create_device() {
	fs_node_t* fb = k_fs_vfs_create_node("fb");
	return fb;
}

void k_dev_fb_set_impl(fb_impl_t* i) {
	impl = i;
	if(impl->stat) {
		impl->stat(&info);
	} else {
		info.w = 80;
		info.h = 64;
		info.cw = 1;
		info.ch = 1;
	}
	k_dev_fb_clear(0);
}

void k_dev_fb_init() {
	k_fs_vfs_mount_node("/dev/fb", __k_dev_fb_create_device());
}

void k_dev_fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
	if(impl && impl->putpixel) {
		fb_pos_t pos = {x, y};
		impl->putpixel(pos, color);
	}
}

static void __k_dev_fb_scroll() {
	pos.y += info.ch;
	if(pos.y >= info.h) {
		pos.y = info.h - info.ch;
		if(impl && impl->scroll) {
			impl->scroll();
		}
	}
}

static void __k_dev_fb_move_right() {
    pos.x += info.cw;
    if (pos.x >= info.w) {
        pos.x = 0;
        __k_dev_fb_scroll();
    }
}

static void __k_dev_fb_move_left() {
	if(!pos.x) {
		return;
	}
    pos.x -= info.cw;
}

void k_dev_fb_putchar(char c, uint32_t fg, uint32_t bg) {
    if (c == '\n') {
        __k_dev_fb_scroll();
    } else if (c == '\r') {
		pos.x = 0;
    } else if (c == '\t') {
        for (int i = 0; i < 3; i++) {
            k_dev_fb_putchar(' ', fg, bg);
        }
	} else if(c == '\b') {
		if(impl && impl->putchar) {
			__k_dev_fb_move_left();
        	impl->putchar(pos, fg, bg, ' ');
		}
	} else {
		if(impl && impl->putchar) {
        	impl->putchar(pos, fg, bg, c);
		}
		__k_dev_fb_move_right();
    }
}

void k_dev_fb_clear(uint32_t color) {
	pos.x = 0;
	pos.y = 0;
	if(impl && impl->clear) {
		impl->clear(color);
	} 
}
