#ifndef __LIB_PNG_H
#define __LIB_PNG_H

#include <stddef.h>
#include <stdint.h>

#define PNG_MAGIC "\x89PNG\x0D\x0A\x1A\x0A"

#define PNG_CHUNK_CR_IHDR_TYPE "IHDR"
#define PNG_CHUNK_CR_IPLT_TYPE "IPLT"
#define PNG_CHUNK_CR_IDAT_TYPE "IDAT"
#define PNG_CHUNK_CR_IEND_TYPE "IEND"

typedef struct {
	size_t w;
	size_t h;
	void* data;
} png_t;

typedef struct {
	uint32_t length;
	char     type[4];
	char     data[];
} png_chunk_t;

typedef struct {
	uint32_t w;
	uint32_t h;
	uint8_t  depth;
	uint8_t  color_type;
	uint8_t  compression;
	uint8_t  filter;
	uint8_t  interlace;
} ihdr_t;

png_t* png_open(const char* path);
void   png_close(png_t* file);
png_t* png_decode(void* data, size_t size);

#endif
