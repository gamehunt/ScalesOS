#include <dev/fb.h>
#include <mem/heap.h>
#include <string.h>

#include "kernel.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "multiboot.h"
#include "util/log.h"

static uint8_t* framebuffer = (uint8_t*)(FRAMEBUFFER_START);
static uint8_t init = 0;

static fb_info_t info;

static uint32_t pos_x = 0;
static uint32_t pos_y = 0;

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

    return K_STATUS_OK;
}

void k_dev_fb_write(char* buff, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        k_dev_fb_putchar(buff[i], 0xFFFFFFFF, 0x0);
    }
}

K_STATUS k_dev_fb_init(multiboot_info_t* mb) {
    if (!(mb->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
        k_warn("Framebuffer info unavailable");
        return K_STATUS_ERR_GENERIC;
    }

    info.width = mb->framebuffer_width;
    info.height = mb->framebuffer_height;
    info.bpp = mb->framebuffer_bpp;
    info.pitch = mb->framebuffer_pitch;
    info.type = mb->framebuffer_type;

    if (info.type != 1 && info.type != 2) {
        k_err("Unsupported frambuffer type!");
        return K_STATUS_ERR_GENERIC;
    }

    if (info.type == 1) {
        if (IS_ERR(__k_dev_fb_psf_init())) {
            k_err("Failed to initialize PSF font.");
            return K_STATUS_ERR_GENERIC;
        }
    }

    k_mem_paging_map_region((uint32_t)framebuffer, mb->framebuffer_addr,
                            info.width * info.height * info.bpp / 8 / 0x1000,
                            0x3, 1);

    k_info("Framebuffer info: %dx%dx%d, type=%d", info.width, info.height,
           info.bpp, info.type);

    init = 1;

    return K_STATUS_OK;
}

uint8_t k_dev_fb_available() { return init; }

void k_dev_fb_putpixel(uint32_t color, uint32_t x, uint32_t y) {
    uint32_t* fb = (uint32_t*)framebuffer;
    if (x < info.width && y < info.height && info.type != 2) {
        fb[info.width * y + x] = color;
    }
}

static void __k_dev_fb_scroll() {
    if (info.type == 2) {
        if (pos_y + 1 < info.height) {
            pos_y++;
        } else {
            memmove(framebuffer, framebuffer + info.pitch * (info.height - 1), info.pitch * info.height);
            memset(framebuffer + (info.height - 1) * info.pitch, 0, info.pitch);
        }
    } else {
        PSF_font* font = (PSF_font*)&_binary_font_psf_start;
        pos_y += font->height;

        if(pos_y >= info.height){
            pos_y = info.height - font->height;
            memmove(framebuffer, framebuffer + font->height * info.pitch, (info.height - font->height) * info.pitch);
            memset(framebuffer + (info.height - font->height) * info.pitch, 0, font->height * info.pitch);
        }
    }
}

static void __k_dev_fb_move_right() {
    if (info.type == 2) {
        pos_x++;
    } else {
        PSF_font* font = (PSF_font*)&_binary_font_psf_start;
        pos_x += font->width;
    }

    if (pos_x >= info.width) {
        pos_x = 0;
        __k_dev_fb_scroll();
    }
}

static void __k_dev_fb_draw_char(uint16_t c, uint32_t cx, uint32_t cy,
                                 uint32_t fg, uint32_t bg) {
    PSF_font* font = (PSF_font*)&_binary_font_psf_start;

    if (unicode && unicode[c]) {
        c = unicode[c];
    }

    uint8_t* glyph =
        (((uint8_t*)font) + font->headersize + c * font->bytesperglyph);

    for (uint32_t y = 0; y < font->height; y++) {
        for (uint32_t x = 0; x < font->width; x++) {
            uint32_t glyph_row = glyph[y];

            if (glyph_row & (1 << (font->width - x))) {
                k_dev_fb_putpixel(fg, cx + x, cy + y);
            } else {
                k_dev_fb_putpixel(bg, cx + x, cy + y);
            }
        }
    }
}

void k_dev_fb_putchar(char c, uint32_t fg, uint32_t bg) {
    if (c == '\n') {
        __k_dev_fb_scroll();
    } else if (c == '\r') {
        pos_x = 0;
    } else if (c == '\t') {
        for (int i = 0; i < 3; i++) {
            k_dev_fb_putchar(' ', fg, bg);
        }
    } else {
        if (info.type == 2) {
            framebuffer[info.pitch * pos_y + pos_x] = c;
        } else {
            __k_dev_fb_draw_char(c, pos_x, pos_y, fg, bg);
        }
        __k_dev_fb_move_right();
    }
}

fb_info_t k_dev_fb_get_info() { return info; }
