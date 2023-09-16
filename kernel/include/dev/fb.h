#ifndef __K_DEV_FB_H
#define __K_DEV_FB_H

#include <stdint.h>

typedef struct {
	uint32_t w;
	uint32_t h;
	uint32_t cw;
	uint32_t ch;
} fb_info_t;

typedef struct {
	uint32_t x;
	uint32_t y;
} fb_pos_t;

typedef void(*fb_clear)(uint32_t color);
typedef void(*fb_putpixel)(fb_pos_t pos, uint32_t color);
typedef void(*fb_putchar)(fb_pos_t pos, uint32_t fg, uint32_t bg, char c);
typedef int32_t(*fb_ctl)(uint32_t request, void* data);
typedef void(*fb_scroll)(void);
typedef void(*fb_stat)(fb_info_t*);

typedef struct {
	fb_scroll   scroll;
	fb_putpixel putpixel;
	fb_putchar  putchar;
	fb_clear    clear;	
	fb_ctl      ctl;
	fb_stat     stat;
} fb_impl_t;

void        k_dev_fb_init();
void        k_dev_fb_set_impl(fb_impl_t* impl);

void 		k_dev_fb_write(char* buff, uint32_t size);
void 		k_dev_fb_putchar(char c, uint32_t fg, uint32_t bg);
void 		k_dev_fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void 		k_dev_fb_clear(uint32_t color);

#endif
