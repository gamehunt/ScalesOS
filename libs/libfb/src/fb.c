#include "fb.h"
#include "kernel/dev/fb.h"
#include "sys/mman.h"
#include "sys/ioctl.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define VERIFY_POINT(fb, x, y) \
	if(x > fb->info.w || y > fb->info.h) { \
		return; \
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

	memcpy(&buf->info, &info, sizeof(fb_info_t));

	(*buf).mem = (void*) r;
}

int fb_open_font(const char* path, fb_font_t* font) {

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
	if(x0 > x1) {
		coord_t tmp = x0;
		x0 = x1;
		x1 = tmp;
	}

	if(y0 > y1) {
		coord_t tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	if(x1 == x0) {
		for(coord_t y = y0; y <= y1; y++) {
			fb_pixel(fb, x0, y, color);
		}
	} else if(y0 == y1) {
		for(coord_t x = x0; x <= x1; x++) {
			fb_pixel(fb, x, y0, color);
		}
	} else {
		for(coord_t x = x0; x <= x1; x++) {
			coord_t y = (y1 - y0) / (x1 - x0) * (x - x0) + y0;
			fb_pixel(fb, x, y, color);
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
