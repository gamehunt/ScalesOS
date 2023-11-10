#ifndef __LIB_PNG_H
#define __LIB_PNG_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint16_t first_entry_index;
	uint16_t color_map_length;
	uint8_t  color_map_entry_size;
}__attribute__((packed, aligned(1))) tga_colormap_info_t;

typedef struct {
	uint16_t x_origin;
	uint16_t y_origin;
	uint16_t w;
	uint16_t h;
	uint8_t  bpp;
	uint8_t  descriptor;
}__attribute__((packed, aligned(1))) tga_image_info_t;

typedef struct {
	uint8_t id_length;
	uint8_t color_map;
	uint8_t image_type;
	tga_colormap_info_t colormap_info;
	tga_image_info_t    image_info;
}__attribute__((packed, aligned(1))) tga_header_t;

typedef struct {
	size_t    w;
	size_t    h;
	uint8_t   bpp;
	uint32_t* data;
} tga_t;

tga_t* tga_open(const char* path);
tga_t* tga_decode(void* data, size_t size);
void   tga_close(tga_t* file);

#endif
