#ifndef __LIBFB_H
#define __LIBFB_H

#include "kernel/dev/fb.h"
#include <stddef.h>
#include <stdio.h>

typedef uint32_t color_t;
typedef int      coord_t;

typedef struct {

} fb_font_t;

typedef struct {
	FILE*     file;
	void*     mem;
	fb_info_t info;
} fb_t;

color_t fb_color(char r, char g, char b);
int  	fb_open(const char* path, fb_t* buf);
int  	fb_open_font(const char* path, fb_font_t* font);
void 	fb_close(fb_t* buffer);
void    fb_close_font(fb_font_t* font);
void 	fb_flush(fb_t* fb);
void    fb_pixel(fb_t* fb, coord_t x0, coord_t y0, color_t color);
void 	fb_line(fb_t* fb, coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color);
void 	fb_rect(fb_t* fb, coord_t x0, coord_t y0, size_t w, size_t h, color_t border);
void 	fb_filled_rect(fb_t* fb, coord_t x0, coord_t y0, size_t w, size_t h, color_t border, color_t fill);
void 	fb_circle(fb_t* fb, coord_t x0, coord_t y0, size_t radius, color_t border);
void 	fb_filled_circle(fb_t* fb, coord_t x0, coord_t y0, size_t radius, color_t border, color_t fill);
void 	fb_ellipse(fb_t* fb, coord_t x0, coord_t y0, size_t a, size_t b, color_t border);
void 	fb_filled_ellipse(fb_t* fb, coord_t x0, coord_t y0, size_t a, size_t b, color_t border, color_t fill);
void 	fb_fill(fb_t* fb, color_t color);

#endif
