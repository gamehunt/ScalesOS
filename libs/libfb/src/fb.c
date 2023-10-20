#include "fb.h"
#include "sys/mman.h"
#include "sys/ioctl.h"
#include "sys/stat.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define VERIFY_POINT(fb, x, y) \
	if(x < 0 || x >= fb->info.w || y < 0 || y >= fb->info.h) { \
		return; \
	} 

static void __fb_swap(coord_t* a, coord_t* b) {
	coord_t tmp = *a;
	*b = *a;
	*a = tmp;
}

color_t fb_color(char r, char g, char b) {
	return (r << 24) | (g << 16) | (b << 8);
}

int fb_open(const char* path, fb_t* buf) {
	FILE* file = fopen(path, "r+");
	if(!file) {
		return -1;
	}

	(*buf).file = file;
	
	fb_info_t info;	
	int r = ioctl(fileno(file), FB_IOCTL_STAT, &info);
	if(r < 0) {
		fclose(file);
		return -1;
	}

	r = (int) mmap(NULL, info.memsz, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file), 0);
	if(r == MAP_FAILED) {
		fclose(file);
		return -1;
	}

	fb_info_t* dst = &buf->info;
	memcpy(dst, &info, sizeof(fb_info_t));

	(*buf).mem = (void*) r;
	return 0;
}

int fb_open_font(const char* path, fb_font_t** font) {
	FILE* f = fopen(path, "r");
	if(!f) {
		return -1;
	}

	struct stat st;
	if(stat(path, &st) < 0) {
		fclose(f);
		return -2;
	}

	int r = (int) mmap(NULL, st.st_size, MAP_SHARED, PROT_READ, fileno(f), 0);
	if(r == MAP_FAILED) {
		fclose(f);
		return -3;
	}

	fb_font_t* _font = (void*) r;

	if(_font->magic != PSF_FONT_MAGIC) {
		fclose(f);
		return -4;
	}

	*font = _font;

	return 0;
}

void fb_close(fb_t* buffer) {
	munmap(buffer->mem, buffer->info.memsz);
	fclose(buffer->file);
}

void fb_close_font(fb_font_t* font) {
}

void fb_flush(fb_t* fb) {
	msync(fb->mem, fb->info.memsz, MS_SYNC);
	ioctl(fileno(fb->file), FB_IOCTL_SYNC, NULL);
}

void fb_pixel(fb_t* fb, coord_t x0, coord_t y0, color_t color) {
	VERIFY_POINT(fb, x0, y0)

	uint32_t* buf = fb->mem;

	buf[y0 * fb->info.w + x0] = color;
}

void fb_line(fb_t* fb, coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color) {
	coord_t dx = abs(x1 - x0);
    coord_t sx = (x0 < x1 ? 1 : -1);
    coord_t dy = -abs(y1 - y0);
    coord_t sy = (y0 < y1 ? 1 : -1);
    coord_t error = dx + dy;
    
	while(1) {
		fb_pixel(fb, x0, y0, color);

		if (x0 == x1 && y0 == y1) {
			break;
		}

		coord_t e2 = 2 * error;
		if(e2 >= dy) {
			if(x0 == x1) {
				break;
			}
			error += dy;
			x0 += sx;
		}

		if(e2 <= dx) {
			if(y0 == y1) {
				break;
			}
            error += dx;
            y0 += sy;
		}
	}
}

void fb_rect(fb_t* fb, coord_t x0, coord_t y0, size_t w, size_t h, color_t border) {
	fb_line(fb, x0, y0, x0 + w, y0, border);
	fb_line(fb, x0 + w, y0, x0 + w, y0 + h, border);
	fb_line(fb, x0 + w, y0 + h, x0, y0 + h, border);
	fb_line(fb, x0, y0 + h, x0, y0, border);
}

void fb_filled_rect(fb_t* fb, coord_t x0, coord_t y0, size_t w, size_t h, color_t border, color_t fill) {
	for(size_t i = 0; i < h; i++) {
		fb_line(fb, x0, y0 + i, x0 + w, y0 + i, fill);
	}
	fb_rect(fb, x0, y0, w, h, border);
}

void fb_circle(fb_t* fb, coord_t x0, coord_t y0, size_t radius, color_t border) {

}

void fb_filled_circle(fb_t* fb, coord_t x0, coord_t y0, size_t radius, color_t border, color_t fill) {

}

void fb_ellipse(fb_t* fb, coord_t x0, coord_t y0, size_t a, size_t b, color_t border) {

}

void fb_filled_ellipse(fb_t* fb, coord_t x0, coord_t y0, size_t a, size_t b, color_t border, color_t fill) {

}

void fb_fill(fb_t* fb, color_t color) {
	ioctl(fileno(fb->file), FB_IOCTL_CLEAR, &color);
}

void fb_char(fb_t* fb, coord_t x0, coord_t y0, char c, fb_font_t* font, color_t b, color_t f) {
    uint8_t* glyph = (((uint8_t*)font) + font->headersize + c * font->bytesperglyph);

    for (uint32_t y = 0; y < font->height; y++) {
        for (uint32_t x = 0; x < font->width; x++) {
            uint32_t glyph_row = glyph[y];

            if (glyph_row & (1 << (font->width - x))) {
                fb_pixel(fb, x0 + x, y0 + y, f);
            } else {
                fb_pixel(fb, x0 + x, y0 + y, b);
            }
        }
    }
}

void fb_string(fb_t* fb, coord_t x, coord_t y, const char* str, fb_font_t* font, color_t b, color_t f) {
	size_t l = strlen(str);
	for(coord_t offs = 0; offs < l; offs++) {
		if(isgraph(str[offs])) {
			fb_char(fb, x + offs * (font->width), y, str[offs], font, b, f);
		}
	}
}
