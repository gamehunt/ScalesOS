#ifndef __LIBFB_H
#define __LIBFB_H

#include "kernel/dev/fb.h"
#include <stddef.h>
#include <stdio.h>

typedef uint32_t color_t;
typedef int      coord_t;

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
} fb_font_t;

typedef struct {
	FILE*     file;
	void*     mem;
	fb_info_t info;
} fb_t;

#define ALPHA(color) (color >> 24)
#define RED(color)   (color >> 16 & 0xFF)
#define BLUE(color)  (color >> 8  & 0xFF)
#define GREEN(color) (color & 0xFF)

color_t fb_color(char r, char g, char b, char a);
double  fb_brightness(color_t);
color_t fb_blend(color_t a, color_t b);
int  	fb_open(const char* path, fb_t* buf);
int  	fb_open_font(const char* path, fb_font_t** font);
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
void    fb_char(fb_t* fb, coord_t x, coord_t y, char c, fb_font_t* font, color_t bg, color_t fg);
void    fb_string(fb_t* fb, coord_t x, coord_t y, const char* str, fb_font_t* font, color_t bg, color_t fg);

#endif
