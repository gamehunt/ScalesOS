#include "png.h"
#include "sys/mman.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>

static uint8_t __png_check_signature(void* data, size_t size) {
	if(size < 8) {
		return 1;
	}
	return memcmp(data, PNG_MAGIC, 8);
}

static uint32_t __png_change_endian(uint32_t value) {
	uint32_t res = 0;
	for(int i = 0; i <= 24; i += 8) {
		res |= (value >> i & 0xFF) << (24 - i);
	}
	return res; 
}

static void* __png_deflate(void* data, size_t size) {
	// TODO
	abort();
}

static void __png_undo_filter(void* data, size_t size) {
	// TODO
	abort();
}

png_t* png_decode(void* data, size_t size) {
	if(__png_check_signature(data, size)) {
		return NULL;
	}	

	png_t* img = malloc(sizeof(png_t));

	png_chunk_t* chunk = (data + 8);

	png_chunk_t* idat_chunks_start = NULL;
	uint32_t     idat_chunk_count = 0;

	void*        img_data = NULL;
	uint32_t     img_data_size = 0;

	uint32_t channels = 0;
	uint32_t bbs      = 0;

	uint32_t color_type = 0;


	while((uint32_t) chunk < (uint32_t) (data + size)) {
		chunk->length = __png_change_endian(chunk->length);
		if(!memcmp(chunk->type, PNG_CHUNK_CR_IHDR_TYPE, 4)) {
			ihdr_t* ihdr = (ihdr_t*) chunk->data;
			img->w = ihdr->w;
			img->h = ihdr->h;

			printf("Bit depth: %d\n", ihdr->depth);
			printf("Color type: %d\n", ihdr->color_type);
			printf("Interlace: %d\n", ihdr->interlace);

			switch(ihdr->color_type) {
				case 0:
				case 3:
					channels = 1;
					break;
				case 4:
					channels = 2;
					break;
				case 2:
					channels = 3;
					break;
				case 6:
					channels = 4;
					break;
			}

			color_type = ihdr->color_type;
			bbs = ihdr->depth;
		}
		else if(!memcmp(chunk->type, PNG_CHUNK_CR_IDAT_TYPE, 4)) {
			if(!idat_chunks_start) {
				idat_chunks_start = chunk;
			}
			idat_chunk_count++;
			img_data_size += chunk->length;
		}
		else if(!memcmp(chunk->type, PNG_CHUNK_CR_IEND_TYPE, 4)) {
			break;
		}
		chunk = (png_chunk_t*) (((uint32_t)chunk) + 12 + chunk->length);
	}

	img_data = malloc(img_data_size);
	uint32_t offset = 0;
	uint32_t chunk_offset = 0;
	for(uint32_t i = 0; i < idat_chunk_count; i++) {
		png_chunk_t* chunk = (png_chunk_t*) (((uint32_t) idat_chunks_start) + chunk_offset);
		memcpy(img_data + offset, chunk->data, chunk->length);
		chunk_offset += 12 + chunk->length;
		offset += chunk->length;
	}
	
	__png_undo_filter(img_data);

	return img;
}

png_t* png_open(const char* path) {
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

	png_t* data = png_decode(mem, st.st_size);

	fclose(file);
	munmap(mem, st.st_size);

	return data;
}

void png_close(png_t* file) {
	free(file);
}
