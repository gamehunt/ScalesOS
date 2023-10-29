#ifndef __K_DEV_FB_H
#define __K_DEV_FB_H

#ifdef __KERNEL
#include "fs/vfs.h"
#else
#include "kernel/fs/vfs.h"
#endif

#include <stdint.h>

#define FB_IOCTL_SYNC  0
#define FB_IOCTL_CLEAR 1
#define FB_IOCTL_STAT  2

typedef struct {
	uint32_t w;
	uint32_t h;
	uint32_t bpp;
	uint32_t memsz;
} fb_info_t;

typedef struct {
	uint32_t x;
	uint32_t y;
} fb_pos_t;

typedef struct {
	uint32_t cw;
	uint32_t ch;
	uint32_t rows;
	uint32_t columns;
} fb_term_info_t;

typedef void(*fb_clear)(uint32_t color);
typedef void(*fb_putpixel)(fb_pos_t pos, uint32_t color);
typedef void(*fb_scroll)(uint32_t pixels);
typedef void(*fb_sync)(void);
typedef void(*fb_release)(void);
typedef void(*fb_init)(fb_info_t*);

typedef struct {
	// fbterm hooks
	fb_init     init;
	fb_scroll   scroll;
	fb_putpixel putpixel;
	fb_clear    clear;	
	fb_sync     sync;
	fb_release  release;

	// fsnode hooks
	fs_ops_t    fs;
} fb_impl_t;

void k_dev_fb_init();
void k_dev_fb_set_impl(fb_impl_t* impl);
void k_dev_fb_write(char* buff, uint32_t size);
void k_dev_fb_putchar(char c, uint32_t fg, uint32_t bg);
void k_dev_fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void k_dev_fb_clear(uint32_t color);
void k_dev_fb_sync();
int  k_dev_fb_ioctl(fs_node_t*, int op, void* arg);
void k_dev_fb_terminfo(fb_term_info_t* info);

#endif
