#include "jpeg.h"
#include "sys/stat.h"
#include "types/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void __change_endian(uint16_t* d) {
	uint8_t* a = (uint8_t*) d;
	uint8_t  tmp = a[0];
	a[0] = a[1];
	a[1] = tmp;
}

static int clamp(int col) {
	if (col > 255) return 255;
	if (col < 0) return 0;
	return col;
}

static void color_conversion(
		float y, float cb, float cr,
		int* r, int* g, int* b
	) {
	float _r = (cr * 1.402 + y);
	float _b = (cb * 1.772 + y);
	float _g = (y - 0.144 * _b - 0.229 * _r) / 0.587;

	*r = clamp(_r + 128);
	*g = clamp(_g + 128);
	*b = clamp(_b + 128);
}

static uint8_t quant[8][64];

static void parse_quant_tables(void* data, size_t len) {
	while(len > 0) {
		uint8_t index = *(uint8_t*)data;
		memcpy(quant[index & 0xF], data + 1, 64);
		data += 65;
		len  -= 65;
	}
}

typedef struct {
	uint8_t  hdr;
	uint16_t height;
	uint16_t width;
	uint8_t  components;
} __attribute__((packed)) dct;

typedef struct {
	uint8_t id;
	uint8_t samp;
	uint8_t qtb_id;
} __attribute__((packed)) quant_mapping;

list_t* quant_mappings = NULL;

static void parse_dct(void* data, size_t len) {
	dct* t = (dct*) data;
	
	uint16_t w = t->width;
	uint16_t h = t->height;

	__change_endian(&w);
	__change_endian(&h);

	t->width  = w;
	t->height = h;

	printf("Size: %dx%d\n", w, h);

	for (int i = 0; i < t->components; ++i) {
		quant_mapping* map = malloc(sizeof(quant_mapping));
		memcpy(&map, data + sizeof(dct), sizeof(quant_mapping));
		if(!quant_mappings) {
			quant_mappings = list_create();
		}
		list_push_front(quant_mappings, map);
	}
}

struct huffman_table {
	uint8_t lengths[16];
	uint8_t elements[256];
} huffman_tables[256] = {0};

static void parse_huffman(void* data, size_t len) {
	while (len > 0) {
		uint8_t hdr = * (uint8_t*) data;
		memcpy(huffman_tables[hdr].lengths, data + 1, 16);

		int o = 0;
		for (int i = 0; i < 16; ++i) {
			int l = huffman_tables[hdr].lengths[i];
			memcpy(&huffman_tables[hdr].elements[o], data + 17 + o, l);
			o += l;
			if(l <= len) {
				len -= l;
			} else {
				len = 0;
			}
		}
	}
}

static int huffman_decode(int code, int bits) {
	int l = 1L << (code - 1);
	if (bits >= l) {
		return bits;
	} else {
		return bits - (2 * l - 1);
	}
}

static void parse_data(void* data, size_t len) {

}

jpeg_t* jpeg_open(const char* path) {
	struct stat st;
	if(stat(path, &st) < 0) {
		return NULL;
	}

	FILE* f = fopen(path, "r");
	if (!f) {
		return NULL;
	}

	void* data = malloc(st.st_size);
	fread(data, st.st_size, 1, f);

	jpeg_t* result = jpeg_decode(data, st.st_size);

	fclose(f);

	return result;
}

void jpeg_close(jpeg_t* file) {

}

jpeg_t* jpeg_decode(void* data, size_t size) {
	off_t offset = 0;
	while(1) {
		uint16_t header = *(uint16_t*)(data + offset);

		__change_endian(&header);

		offset += 2;

		printf("Mark: 0x%.4x", header);

		if (header == 0xffd8) {
			printf(" -- Start\n");
			continue;
		} else if (header == 0xffd9) {
			printf(" -- End\n");
			break;
		} else {
			uint16_t length = *(uint16_t*)(data + offset);

			__change_endian(&length);

			length -= 2;
			offset += 2;

 			if (header == 0xffdb) { //quant table
				printf(" -- QUANT TABLE\n");
				parse_quant_tables(data + offset, length);
			} else if (header == 0xffc0) { // dct
				printf(" -- DCT\n");
				parse_dct(data + offset, length);
			} else if (header == 0xffc4) { // hufman
				printf(" -- HUFFMAN\n");
				parse_huffman(data + offset, length);
			} else if (header == 0xffda) { // data
				printf(" -- DATA\n");
				parse_data(data + offset, length);
				break;
			} else {
				printf(" -- UNKNOWN\n");
			}

			offset += length;
		}
	}

	return NULL;
}
