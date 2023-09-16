#include "video/lfbgeneric.h"
#include "dev/fb.h"
#include "kernel.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "util/log.h"
#include <stddef.h>
#include <string.h>

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

static K_STATUS __k_video_generic_lfb_psf_init() {
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

    return K_STATUS_OK;
}

typedef struct lfb_info{
    uint8_t  type;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
} lfb_info_t;

static lfb_info_t info;
static uint8_t* framebuffer = (uint8_t*) FRAMEBUFFER_START;

static void __k_video_lfb_scroll() {
    if (info.type == 2) {
        memmove(framebuffer, framebuffer + info.pitch * (info.height - 1), info.pitch * info.height);
        memset(framebuffer + (info.height - 1) * info.pitch, 0, info.pitch);
    } else {
        PSF_font* font = (PSF_font*)&_binary_font_psf_start;
        memmove(framebuffer, framebuffer + font->height * info.pitch, (info.height - font->height) * info.pitch);
        memset(framebuffer + (info.height - font->height) * info.pitch, 0, font->height * info.pitch);
    }
}

static void __k_video_lfb_putpixel(fb_pos_t pos, uint32_t color) {
    uint32_t* fb = (uint32_t*) framebuffer;
    if (pos.x < info.width && pos.y < info.height && info.type != 2) {
        fb[info.width * pos.y + pos.x] = color;
    }
}

static void __k_video_lfb_draw_char(fb_pos_t pos, uint32_t fg, uint32_t bg, uint16_t c) {
    PSF_font* font = (PSF_font*)&_binary_font_psf_start;

    if (unicode && unicode[c]) {
        c = unicode[c];
    }

    uint8_t* glyph = (((uint8_t*)font) + font->headersize + c * font->bytesperglyph);

    for (uint32_t y = 0; y < font->height; y++) {
        for (uint32_t x = 0; x < font->width; x++) {
            uint32_t glyph_row = glyph[y];

			fb_pos_t new_pos = {pos.x + x, pos.y + y};
            if (glyph_row & (1 << (font->width - x))) {
                __k_video_lfb_putpixel(new_pos, fg);
            } else {
                __k_video_lfb_putpixel(new_pos, bg);
            }
        }
    }
}

static void __k_video_lfb_put_char(fb_pos_t pos, uint32_t fg, uint32_t bg, char c) {
	__k_video_lfb_draw_char(pos, fg, bg, (uint16_t) c);
}

static void __k_video_lfb_stat(fb_info_t* fb_info) {
    PSF_font* font = (PSF_font*)&_binary_font_psf_start;
	fb_info->w = info.width; 
	fb_info->h = info.height;
	fb_info->cw = font->width;
	fb_info->ch = font->height;
}

static void __k_video_lfb_clear(uint32_t color) {
	memset(framebuffer, color, info.width * info.height * info.bpp / 8);
}

static uint32_t __k_video_lfb_write(fs_node_t* node UNUSED, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	uint32_t max = info.width * info.height * info.bpp / 8;
	
	if(offset >= max) {
		return 0;	
	}

	if(offset + size >= max) {
		size = max - offset;
	}

	memcpy(framebuffer + offset, buffer, size);

	return size;
}

void k_video_generic_lfb_init(multiboot_info_t* mb) {
	if (!(mb->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
        k_warn("Framebuffer info unavailable");
		return;
    }

    info.width = mb->framebuffer_width;
    info.height = mb->framebuffer_height;
    info.bpp = mb->framebuffer_bpp;
    info.pitch = mb->framebuffer_pitch;
    info.type = mb->framebuffer_type;

    if (info.type != 1 && info.type != 2) {
        k_err("Unsupported frambuffer type!");
		return;
    }

    if (info.type == 1) {
		__k_video_generic_lfb_psf_init();
    }

    k_mem_paging_map_region((uint32_t)framebuffer, mb->framebuffer_addr,
                            info.width * info.height * info.bpp / 8 / 0x1000,
                            PAGE_PRESENT | PAGE_WRITABLE, 1);

    k_info("Framebuffer info: %dx%dx%d, type=%d", info.width, info.height,
           info.bpp, info.type);

	fb_impl_t* fb_impl = k_malloc(sizeof(fb_impl_t));
	
	fb_impl->scroll   = &__k_video_lfb_scroll;
	fb_impl->putpixel = &__k_video_lfb_putpixel;
	fb_impl->putchar  = &__k_video_lfb_put_char;
	fb_impl->stat     = &__k_video_lfb_stat;
	fb_impl->clear    = &__k_video_lfb_clear;

	fb_impl->fs.write = &__k_video_lfb_write;

	k_dev_fb_set_impl(fb_impl);
}
