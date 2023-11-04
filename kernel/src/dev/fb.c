#include <dev/fb.h>
#include <mem/heap.h>
#include <stdio.h>
#include <string.h>

#include "dev/serial.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "kernel/fs/vfs.h"
#include "util/log.h"
#include "mem/paging.h"
#include "errno.h"

static fb_pos_t   pos  = {0, 0};
static fb_impl_t* impl = 0;
static fb_info_t  info;
static fs_node_t* fb   = 0;

static uint32_t cw = 0;
static uint32_t ch = 0;

void k_dev_fb_write(char* buff, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        k_dev_fb_putchar(buff[i], 0xFFFFFFFF, 0x0);
    }
}

void k_dev_fb_set_impl(fb_impl_t* i) {
	if(impl) {
		fb_impl_t* tmp = impl;
		impl = NULL;
		if(tmp->release) {
			tmp->release();
		}
		k_free(tmp);
	}
	impl = i;
	if(impl->init) {
		impl->init(&info);
	} else {
		info.w = 80;
		info.h = 64;
	}
	fb->fs = i->fs;
	k_dev_fb_clear(0);
}

void k_dev_fb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
	if(impl && impl->putpixel) {
		fb_pos_t pos = {x, y};
		impl->putpixel(pos, color);
	}
}

static void __k_dev_fb_scroll() {
	pos.y += ch;
	if(pos.y >= info.h) {
		pos.y = info.h - ch;
		if(impl && impl->scroll) {
			impl->scroll(ch);
		}
	}
}

static void __k_dev_fb_move_right() {
    pos.x += cw;
    if (pos.x >= info.w) {
        pos.x = 0;
        __k_dev_fb_scroll();
    }
}

static void __k_dev_fb_move_left() {
	if(!pos.x) {
		return;
	}
    pos.x -= cw;
}

extern char _binary_font_psf_start;
extern char _binary_font_psf_end;

uint16_t* unicode = 0;

#define PSF_FONT_MAGIC 0x864ab572

typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} PSF_font;

static K_STATUS __k_dev_fb_psf_init() {
    /* cast the address to PSF header struct */
    PSF_font* font = (PSF_font*)&_binary_font_psf_start;

    if (font->magic != PSF_FONT_MAGIC) {
        return K_STATUS_ERR_GENERIC;
    }

    /* is there a unicode table? */
    if (!font->flags) {
        unicode = NULL;
        return K_STATUS_OK;
    }

    uint16_t glyph = 0;

    // get the offset of the table 
    char *s = (char*)(
    (unsigned char*)&_binary_font_psf_start +
      font->headersize +
      font->numglyph * font->bytesperglyph
    );
    // allocate memory for translation table 
    unicode = k_calloc(0xFFFF, 2);
    while(s < &_binary_font_psf_end) {
        uint16_t uc = (uint16_t)((unsigned char *)s[0]);;
        if(uc == 0xFF) {
            glyph++;
            s++;
            continue;
        } else if(uc & 128) {
            if((uc & 32) == 0 ) {
                uc = ((s[0] & 0x1F)<<6)+(s[1] & 0x3F);
                s++;
            } else
            if((uc & 16) == 0 ) {
                uc = ((((s[0] & 0xF)<<6)+(s[1] & 0x3F))<<6)+(s[2] & 0x3F);
                s+=2;
            } else
            if((uc & 8) == 0 ) {
                uc = ((((((s[0] & 0x7)<<6)+(s[1] & 0x3F))<<6)+(s[2] & 0x3F))<<6)+(s[3] & 0x3F);
                s+=3;
            } else
                uc = 0;
        }
        unicode[uc] = glyph;
        s++;
    }

	cw = font->width;
	ch = font->height;

    return K_STATUS_OK;
}

static void __k_dev_fb_draw_char(fb_pos_t pos, uint32_t fg, uint32_t bg, uint16_t c) {
    PSF_font* font = (PSF_font*)&_binary_font_psf_start;

    if (unicode && unicode[c]) {
        c = unicode[c];
    }

    uint8_t* glyph = (((uint8_t*)font) + font->headersize + c * font->bytesperglyph);

    for (uint32_t y = 0; y < font->height; y++) {
        for (uint32_t x = 0; x < font->width; x++) {
            uint32_t glyph_row = glyph[y];

            if (glyph_row & (1 << (font->width - x))) {
                k_dev_fb_putpixel(pos.x + x, pos.y + y, fg);
            } else {
                k_dev_fb_putpixel(pos.x + x, pos.y + y, bg);
            }
        }
    }
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
		__k_dev_fb_move_left();
		__k_dev_fb_draw_char(pos, fg, bg, ' ');
	} else {
		__k_dev_fb_draw_char(pos, fg, bg, c);
		__k_dev_fb_move_right();
    }
}

void k_dev_fb_clear(uint32_t color) {
	pos.x = 0;
	pos.y = 0;
	if(impl) {
		if(impl->clear) {
			impl->clear(color);
		} 
	}
}


static fs_node_t* __k_dev_fb_create_device() {
	fs_node_t* fb = k_fs_vfs_create_node("fb");
	return fb;
}

int k_dev_fb_ioctl(fs_node_t* node UNUSED, int request, void* args) {
	uint32_t arg; 
	switch(request) {
		case FB_IOCTL_CLEAR:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			arg = *((uint32_t*) args);
			k_dev_fb_clear(arg);
			return 0;
		case FB_IOCTL_STAT:
			if(!IS_VALID_PTR((uint32_t) args)) {
				return -EINVAL;
			}
			memcpy(args, &info, sizeof(fb_info_t));
			return 0;
		default:
			k_warn("Unknown fb ioctl: %d", request);
			return -EINVAL;
	}                  
}                      
                  	  
void k_dev_fb_init() {
	if(!IS_OK(__k_dev_fb_psf_init())) {
		k_warn("Faled to load kernel font.");
	}             
	fb =  __k_dev_fb_create_device();
	k_fs_vfs_mount_node("/dev/fb", fb);
}

void k_dev_fb_terminfo(fb_term_info_t* inf) {
	inf->cw = cw;
	inf->ch = ch;
	inf->rows = info.w / cw;
	inf->columns = info.h / ch;
}

void k_dev_fb_restore_text(char* buff, uint32_t size) {
	int32_t  offset = 0;
	uint32_t lines  = 0;
	for(offset = size - 1; offset >= 0; offset--) {
		if(buff[offset] == '\n') {
			lines++;
			if(lines >= info.h / ch) {
				break;
			}
		}
	}
	if(offset < 0) {
		offset = 0;
	}
	for(uint32_t i = offset; i < size; i++) {
		k_dev_fb_putchar(buff[i], 0xFFFFFFFF, 0x0);
	}
}

void k_dev_fb_restore(fb_save_data_t* sav) {
	if(impl && impl->restore) {
		impl->restore(sav);
	}
}

void k_dev_fb_save(fb_save_data_t* sav) {
	if(impl && impl->save) {
		impl->save(sav);
	}
}
