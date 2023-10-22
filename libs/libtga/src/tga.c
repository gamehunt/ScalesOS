#include "tga.h"
#include "sys/mman.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

static uint8_t tga_check_signature(void* data, size_t size) {
	if(size < 18) {
		return 0;
	}

	return strcmp((data + size - 18), "TRUEVISION-XFILE.");
}

tga_t* tga_decode(void* data, size_t size) {
	// if(tga_check_signature(data, size)) {
	// 	printf("Not a tga.\n");
	// 	return NULL;
	// }

	tga_header_t* hdr = data;

	printf("Image type: %d\n", hdr->image_type);
	printf("Image width: %d\n", hdr->image_info.w);
	printf("Image height: %d\n", hdr->image_info.h);
	printf("Image bpp: %d\n", hdr->image_info.bpp);

	void* color_map_data = data + sizeof(tga_header_t) + hdr->id_length;
	void* image_data     = color_map_data + hdr->colormap_info.color_map_length / 8 * hdr->colormap_info.color_map_entry_size;

	if(hdr->image_type == 0 || hdr->image_type > 3) {
		printf("Unsupported image type: %d\n", hdr->image_type);
		return NULL;
	} 

	tga_t* img = malloc(sizeof(tga_t));

	img->w     = hdr->image_info.w;
	img->h     = hdr->image_info.h;

	img->data  = malloc(img->w * img->h * 4);

	uint32_t bpp = hdr->image_info.bpp;

	for(int y = 0; y < img->h; y++) {
		for(int x = 0; x < img->w; x++) {
			uint32_t index = y * img->w + x;
			if(hdr->image_type == 2) {
				memcpy(img->data + index, image_data + index * bpp / 8, bpp / 8);
				if(bpp < 32) {
					img->data[index] |= 0xFF000000;
				}
			} else if(hdr->image_type == 3) {
				uint8_t value = *((uint8_t*)image_data + index);
				img->data[index] = 0xFF000000 | (value << 16) | (value << 8) | value; 
			} else if(hdr->image_type == 1) {
				uint16_t value = *((uint16_t*)(image_data + index * bpp / 8));
				img->data[index] = 0xFF000000 | *(uint32_t*)(color_map_data + value * hdr->colormap_info.color_map_entry_size / 8);
			}
		}
	}

	return img;
}

tga_t* tga_open(const char* path) {
	struct stat st;
	if(stat(path, &st) < 0) {
		return NULL;
	}

	FILE* file = fopen(path, "r");
	if(!file) {
		return NULL;
	}

	void* mem = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(file), 0);
	if((int32_t) mem == MAP_FAILED) {
		fclose(file);
		return NULL;
	}

	tga_t* data = tga_decode(mem, st.st_size);

	fclose(file);
	munmap(mem, st.st_size);

	return data;
}

void tga_close(tga_t* file) {
	free(file->data);
	free(file);
}
