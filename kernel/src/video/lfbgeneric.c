#include "video/lfbgeneric.h"
#include "dev/fb.h"
#include "dev/serial.h"
#include "kernel.h"
#include "kernel/mem/memory.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "util/log.h"
#include <stddef.h>
#include <string.h>

typedef struct lfb_info{
    uint8_t  type;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
	uint32_t phys;
} lfb_info_t;

static lfb_info_t info;
static uint32_t framebuffer_size = 0;
static uint8_t* framebuffer = (uint8_t*) FRAMEBUFFER_START;
static uint8_t* backbuffer  = NULL;

static void __k_video_lfb_scroll(uint32_t pixels) {
    if (info.type == 2) {
        memmove(backbuffer, backbuffer + info.pitch * (info.height - 1), info.pitch * info.height);
        memset(backbuffer + (info.height - 1) * info.pitch, 0, info.pitch);
    } else {
        memmove(backbuffer, backbuffer + pixels * info.pitch, (info.height - pixels) * info.pitch);
        memset(backbuffer + (info.height - pixels) * info.pitch, 0, pixels * info.pitch);
    }
}

static void __k_video_lfb_putpixel(fb_pos_t pos, uint32_t color) {
    uint32_t* fb = (uint32_t*) backbuffer;
    if (pos.x < info.width && pos.y < info.height && info.type != 2) {
        fb[info.width * pos.y + pos.x] = color;
    }
}

static void __k_video_lfb_init(fb_info_t* fb_info) {
	fb_info->w = info.width; 
	fb_info->h = info.height;
    k_mem_paging_map_region((uint32_t)framebuffer, info.phys,
                            framebuffer_size / 0x1000,
                            PAGE_PRESENT | PAGE_WRITABLE, 1);
}

static void __k_video_lfb_clear(uint32_t color) {
	uint32_t* buffer = (uint32_t*) backbuffer;
	for(uint32_t i = 0; i < framebuffer_size / 4; i++) {
		buffer[i] = color;
	} 
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

	memcpy(backbuffer + offset, buffer, size);

	return size;
}

static void __k_video_lfb_sync() {
	memcpy(framebuffer, backbuffer, framebuffer_size);
}

static void __k_video_lfb_release() {
	k_mem_paging_unmap_region((uint32_t) framebuffer, framebuffer_size / 0x1000);
	k_free(backbuffer);
}

void k_video_generic_lfb_init(multiboot_info_t* mb) {
	if (!(mb->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
        k_warn("Framebuffer info unavailable");
		return;
    }

    info.width  = mb->framebuffer_width;
    info.height = mb->framebuffer_height;
    info.bpp    = mb->framebuffer_bpp;
    info.pitch  = mb->framebuffer_pitch;
    info.type   = mb->framebuffer_type;
	info.phys   = mb->framebuffer_addr;

    if (info.type != 1 && info.type != 2) {
        k_err("Unsupported frambuffer type!");
		return;
    }

	framebuffer_size = info.width * info.height * info.bpp / 8;


	backbuffer = k_malloc(framebuffer_size);

    k_info("Framebuffer info: %dx%dx%d, type=%d", info.width, info.height,
           info.bpp, info.type);

	fb_impl_t* fb_impl = k_malloc(sizeof(fb_impl_t));
	
	fb_impl->scroll   = &__k_video_lfb_scroll;
	fb_impl->putpixel = &__k_video_lfb_putpixel;
	fb_impl->clear    = &__k_video_lfb_clear;
	fb_impl->sync     = &__k_video_lfb_sync;
	fb_impl->release  = &__k_video_lfb_release;
	fb_impl->init     = &__k_video_lfb_init;

	fb_impl->fs.write = &__k_video_lfb_write;
	fb_impl->fs.ioctl = (fs_ioctl_t) &k_dev_fb_ioctl;

	k_dev_fb_set_impl(fb_impl);
}
