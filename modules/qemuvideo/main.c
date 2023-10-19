#include "dev/pci.h"
#include "mem/memory.h"
#include <kernel/mem/heap.h>
#include <kernel/mem/paging.h>
#include <kernel/mod/modules.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>
#include <kernel/dev/fb.h>
#include <kernel/util/asm_wrappers.h>
#include <string.h>

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF

#define VBE_DISPI_INDEX_ID          (0)
#define VBE_DISPI_INDEX_XRES        (1)
#define VBE_DISPI_INDEX_YRES        (2)
#define VBE_DISPI_INDEX_BPP         (3)
#define VBE_DISPI_INDEX_ENABLE      (4)
#define VBE_DISPI_INDEX_BANK        (5)
#define VBE_DISPI_INDEX_VIRT_WIDTH  (6)
#define VBE_DISPI_INDEX_VIRT_HEIGHT (7)
#define VBE_DISPI_INDEX_X_OFFSET    (8)
#define VBE_DISPI_INDEX_Y_OFFSET    (9)

#define BACKBUFFER_SOFT 0
#define BACKBUFFER_HARD 1

typedef struct {
	fb_impl_t impl;
	uint32_t  buffer_phys;
	uint32_t  buffer_size;
	uint8_t*  buffer;
	uint8_t*  backbuffer;
	uint8_t   backbuffer_type;
	uint8_t   offset;
	uint32_t  width;
	uint32_t  height;
	uint32_t  bpp;
	uint32_t  pitch;
} qemu_lfb_t;

static qemu_lfb_t __impl;

static pci_device_t* detect_qemu_lfb() {
	return k_dev_pci_find_device_by_vendor(0x1234, 0x1111);
}

static uint16_t __bga_read(uint16_t index) {
	outw(VBE_DISPI_IOPORT_INDEX, index);
	return inw(VBE_DISPI_IOPORT_DATA);
}

static void __bga_write(uint16_t index, uint16_t data) {
	outw(VBE_DISPI_IOPORT_INDEX, index);
	outw(VBE_DISPI_IOPORT_DATA, data);
}

static uint8_t __bga_check_version() {
	return __bga_read(VBE_DISPI_INDEX_ID) >= 0xB0C2;
}

static void __qemu_fb_clear(uint32_t color) {
	uint32_t* buffer = (uint32_t*) __impl.backbuffer;
	for(uint32_t i = 0; i < __impl.buffer_size / 4; i++) {
		buffer[i] = color;
	} 
}

static void __qemu_fb_putpixel(fb_pos_t pos, uint32_t color) {
    uint32_t* fb = (uint32_t*) __impl.backbuffer;
    if (pos.x < __impl.width && pos.y < __impl.height) {
        fb[__impl.width * pos.y + pos.x] = color;
    }
}

static void __qemu_fb_scroll(uint32_t pixels) {
    memmove(__impl.backbuffer, __impl.backbuffer + pixels * __impl.pitch, (__impl.height - pixels) * __impl.pitch);
    memset(__impl.backbuffer + (__impl.height - pixels) * __impl.pitch, 0, pixels * __impl.pitch);
}

static void __qemu_fb_sync() {
	memcpy(__impl.buffer, __impl.backbuffer, __impl.buffer_size);
	if(__impl.backbuffer_type == BACKBUFFER_HARD) {
		if(__impl.offset) {
			__impl.buffer = (uint8_t*) (FRAMEBUFFER_START + __impl.buffer_size);
			__bga_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
		} else {
			__impl.buffer = (uint8_t*) (FRAMEBUFFER_START);
			__bga_write(VBE_DISPI_INDEX_Y_OFFSET, __impl.height);
		}
		__impl.offset = !__impl.offset;
	}
}

static void __qemu_fb_release() {
	k_free(__impl.backbuffer);
	if(__impl.backbuffer_type == BACKBUFFER_SOFT) {
		k_mem_paging_unmap_region(FRAMEBUFFER_START, __impl.buffer_size / 0x1000);
	} else {
		k_mem_paging_unmap_region(FRAMEBUFFER_START, (__impl.buffer_size * 2) / 0x1000);
	}
}

static void __qemu_fb_init(fb_info_t* info) {
	info->w = __impl.width;
	info->h = __impl.height;
	info->bpp = __impl.bpp;
	info->memsz = __impl.buffer_size;
	if(__impl.backbuffer_type == BACKBUFFER_SOFT) {
		k_mem_paging_map_region(FRAMEBUFFER_START, __impl.buffer_phys, __impl.buffer_size / 0x1000, PAGE_PRESENT | PAGE_WRITABLE, 1);
	} else {
		k_mem_paging_map_region(FRAMEBUFFER_START, __impl.buffer_phys, (__impl.buffer_size * 2) / 0x1000, PAGE_PRESENT | PAGE_WRITABLE, 1);
	}
}

static uint32_t __qemu_fb_write(fs_node_t* node UNUSED, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	uint32_t max = __impl.buffer_size;
	
	if(offset >= max) {
		return 0;	
	}

	if(offset + size >= max) {
		size = max - offset;
	}

	memcpy(__impl.backbuffer + offset, buffer, size);

	return size;
}

static uint32_t __qemu_fb_read(fs_node_t* node UNUSED, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	uint32_t max = __impl.buffer_size;
	
	if(offset >= max) {
		return 0;	
	}

	if(offset + size >= max) {
		size = max - offset;
	}

	memcpy(buffer, __impl.backbuffer + offset, size);

	return size;
}

static void __init_fb_impl(pci_device_t* device) {
	if(!__bga_check_version()) {
		k_err("BGA version is too low!");
		return;
	}

	memset(&__impl, 0, sizeof(qemu_lfb_t));

	uint16_t w   = __bga_read(VBE_DISPI_INDEX_XRES);
	uint16_t h   = __bga_read(VBE_DISPI_INDEX_YRES);
	uint16_t bpp = __bga_read(VBE_DISPI_INDEX_BPP);
	uint16_t vh  = __bga_read(VBE_DISPI_INDEX_VIRT_HEIGHT);

	__impl.width  = w;
	__impl.height = h;
	__impl.bpp    = bpp;
	__impl.pitch  = w * bpp / 8;

	__impl.buffer_size = w * h * bpp / 8;
	__impl.buffer_phys = device->bars[0] & 0xFFFFFFF0;
	__impl.buffer      = (uint8_t*) FRAMEBUFFER_START;

	__impl.backbuffer = k_malloc(__impl.buffer_size);
	if(vh >= __impl.height * 2) {
		__impl.backbuffer_type = BACKBUFFER_HARD;
		__impl.buffer += __impl.buffer_size;
	} else {
		__impl.backbuffer_type = BACKBUFFER_SOFT;
	}

	__impl.impl.clear    = &__qemu_fb_clear;
	__impl.impl.putpixel = &__qemu_fb_putpixel;
	__impl.impl.scroll   = &__qemu_fb_scroll;
	__impl.impl.sync     = &__qemu_fb_sync;
	__impl.impl.release  = &__qemu_fb_release;
	__impl.impl.init     = &__qemu_fb_init;

	__impl.impl.fs.write = &__qemu_fb_write;
	__impl.impl.fs.read  = &__qemu_fb_read;
	__impl.impl.fs.ioctl = &k_dev_fb_ioctl;

	k_dev_fb_set_impl(&__impl.impl);
}

K_STATUS load(){
	pci_device_t* dev = detect_qemu_lfb();
	if(!dev) {
		k_err("QEMU lfb device not detected.");
		return K_STATUS_ERR_GENERIC;
	}

	__init_fb_impl(dev);

    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("qemuvideo", &load, &unload)
