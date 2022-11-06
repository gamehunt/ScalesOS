#ifndef __K_DEV_FB_H
#define __K_DEV_FB_H

#include "kernel.h"
#include "multiboot.h"

typedef struct fb_info{
    uint8_t  type;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
}fb_info_t;

K_STATUS    k_dev_fb_init(multiboot_info_t* mb);
uint8_t     k_dev_fb_available();

fb_info_t   k_dev_fb_get_info();

void        k_dev_fb_putchar(char c, uint32_t fg, uint32_t bg);
void        k_dev_fb_putpixel(uint32_t color, uint32_t x, uint32_t y);

#endif