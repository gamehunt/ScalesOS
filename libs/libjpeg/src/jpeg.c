#include "jpeg.h"
#include "sys/stat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
	uint8_t lengths[16];
	uint8_t elements[256];
} __attribute__((packed)) huffman_table; 

typedef struct {
	float base[64];
} idct;

typedef struct {
	huffman_table  huffman_tables[256];

	uint8_t        quant[8][64];
	quant_mapping* quant_mappings[3];

	size_t w;
	size_t h;

	uint32_t* data;
} __attribute__((packed)) __jpeg_internal;

static void __change_endian(uint16_t* d) {
	uint8_t* a = (uint8_t*) d;
	uint8_t  tmp = a[0];
	a[0] = a[1];
	a[1] = tmp;
}

static const int zigzag[] = {
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

static const float cosines[8][8] = {
	{ 0.35355339059,0.35355339059,0.35355339059,0.35355339059,0.35355339059,0.35355339059,0.35355339059,0.35355339059 },
	{ 0.490392640202,0.415734806151,0.27778511651,0.0975451610081,-0.0975451610081,-0.27778511651,-0.415734806151,-0.490392640202 },
	{ 0.461939766256,0.191341716183,-0.191341716183,-0.461939766256,-0.461939766256,-0.191341716183,0.191341716183,0.461939766256 },
	{ 0.415734806151,-0.0975451610081,-0.490392640202,-0.27778511651,0.27778511651,0.490392640202,0.0975451610081,-0.415734806151 },
	{ 0.353553390593,-0.353553390593,-0.353553390593,0.353553390593,0.353553390593,-0.353553390593,-0.353553390593,0.353553390593 },
	{ 0.27778511651,-0.490392640202,0.0975451610081,0.415734806151,-0.415734806151,-0.0975451610081,0.490392640202,-0.27778511651 },
	{ 0.191341716183,-0.461939766256,0.461939766256,-0.191341716183,-0.191341716183,0.461939766256,-0.461939766256,0.191341716183 },
	{ 0.0975451610081,-0.27778511651,0.415734806151,-0.490392640202,0.490392640202,-0.415734806151,0.27778511651,-0.0975451610081 },
};

static float precalc_table[8][8][8][8]= {{{{0}}}};

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

static void parse_quant_tables(__jpeg_internal* states, void* data, size_t len) {
	int l = len;
	while(l > 0) {
		uint8_t index = *(uint8_t*)data;
		memcpy(states->quant[index & 0xF], data + 1, 64);
		data += 65;
		l    -= 65;
	}
}


static void parse_dct(__jpeg_internal* states, void* data, size_t len) {
	dct* t = (dct*) data;
	
	uint16_t w = t->width;
	uint16_t h = t->height;

	__change_endian(&w);
	__change_endian(&h);

	// t->width  = w;
	// t->height = h;

	printf("Size: %dx%d, components=%d\n", w, h, t->components);

	states->w = w;
	states->h = h;

	for (int i = 0; i < t->components; ++i) {
		quant_mapping* map = malloc(sizeof(quant_mapping));
		memcpy(map, data + sizeof(dct) + i * sizeof(quant_mapping), sizeof(quant_mapping));
		states->quant_mappings[i] = map;
		printf("%d %d %d\n", map->id, map->samp, map->qtb_id);
	}
}

static void parse_huffman(__jpeg_internal* states, void* data, size_t len) {
	int   llen = len;
	off_t offset = 0;
	while (llen > 0) {
		uint8_t hdr = *(uint8_t*) (data + offset);
		memcpy(states->huffman_tables[hdr].lengths, data + offset + 1, 16);

		llen   -= 17;
		offset += 17;

		int o = 0;
		for (int i = 0; i < 16; ++i) {
			int l = states->huffman_tables[hdr].lengths[i];
			memcpy(&states->huffman_tables[hdr].elements[o], data + offset, l);

			o      += l;
			offset += l;
			llen   -= l;

		}
	}
}

static int huffman_decode(int code, int bits) {
	int l = (1L << (code - 1));
	if (bits >= l) {
		return bits;
	} else {
		return bits - (2 * l - 1);
	}
}

static void build_table() {
	for (int n = 0; n < 8; ++n) {
		for (int m = 0; m < 8; ++m) {
			for (int y = 0; y < 8; ++y) {
				for (int x = 0; x < 8; ++x) {
					precalc_table[n][m][y][x] = cosines[n][x] * cosines[m][y];
				}
			}
		}
	}
}

typedef struct {
	void*   data;
	size_t  len;
	off_t   byte;
	uint8_t bit;
} bit_stream;

static int get_bit(bit_stream* stream) {
	if(stream->byte >= stream->len) {
		return 0;
	}

	uint8_t byte = *(uint8_t*)(stream->data + stream->byte);
	int bit = (byte >> (7 - stream->bit)) & 1;

	stream->bit++;
	if(stream->bit >= 8) {
		stream->bit = 0;
		if(byte == 0xFF) {
			if(*(uint8_t*)(stream->data + stream->byte + 1)) {
				stream->byte = stream->len;
			} else {
				stream->byte += 2;
			}
		} else {
			stream->byte++;
		}
	}

	return bit;
}

static int get_bitn(bit_stream* stream, int l) {
	int val = 0;
	for (int i = 0; i < l; ++i) {
		val = val * 2 + get_bit(stream);
	}
	return val;
}

static int huffman_get_code(huffman_table* table, bit_stream* stream) {
	int val = 0;
	int off = 0;
	int ini = 0;

	for (int i = 0; i < 16; ++i) {
		val = val * 2 + get_bit(stream);
		if (table->lengths[i] > 0) {
			if (val - ini < table->lengths[i]) {
				return table->elements[off + val - ini];
			}
			ini += table->lengths[i];
			off += table->lengths[i];
		}
		ini *= 2;
	}

	return -1;
}

 static void add_idc(idct * self, int n, int m, int coeff) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			self->base[x + 8 * y] += precalc_table[n][m][y][x] * coeff;
		}
	}
 }

static void add_zigzag(idct* self, int zi, int coeff) {
	if(zi >= 64) {
		return;
	}
	int i = zigzag[zi];
	int n = i & 0x7;
	int m = i >> 3;
	add_idc(self, n, m, coeff);
}

static idct* build_matrix(__jpeg_internal* jpeg, idct* i, bit_stream* bs, int idx, uint8_t* quant, int oldcoeff, int* outcoeff) {
	memset(i, 0, sizeof(idct));

	int code    = huffman_get_code(&jpeg->huffman_tables[idx], bs);
	int bits    = get_bitn(bs, code);
	int dccoeff = huffman_decode(code, bits) + oldcoeff;

	add_zigzag(i, 0, dccoeff * quant[0]);
	int l = 1;

	while (l < 64) {
		code = huffman_get_code(&jpeg->huffman_tables[16 + idx], bs);
		if (code <= 0) break;
		if (code > 15) {
			l += (code >> 4);
			code = code & 0xF;
		}
		bits = get_bitn(bs, code);
		if(l < 64) {
			int coeff = huffman_decode(code, bits);
			add_zigzag(i, l, coeff * quant[l]);
			l += 1;
		}
	}

	*outcoeff = dccoeff;
	return i;
}

static void draw_matrix(__jpeg_internal* jpeg, int x, int y, idct* L, idct* cb, idct* cr) {
	for (int yy = 0; yy < 8; ++yy) {
		for (int xx = 0; xx < 8; ++xx) {
			int o = xx + 8 * yy;
			int r, g, b;
			color_conversion(L->base[o], cb->base[o], cr->base[o], &r, &g, &b);
			uint32_t c = 0xFF000000 | (r << 16) | (g << 8) | b;
			int xp = x * 8 + xx;
			int yp = y * 8 + yy;
			jpeg->data[xp + yp * jpeg->w] = c;
		}
	}
}

static void parse_data(__jpeg_internal* jpeg, void* data, size_t len, size_t end) {
	if(precalc_table[0][0][0][0] == 0.0) {
		build_table();
	}

	jpeg->data = calloc(1, jpeg->w * jpeg->h * 4);

 	int old_lum = 0;
	int old_crd = 0;
	int old_cbd = 0;

	bit_stream bs;
	bs.data = data;
	bs.len  = end;
	bs.byte = len;
	bs.bit  = 0;

	for (int y = 0; y < jpeg->h / 8 + !!(jpeg->h & 0x7); ++y) {
		for (int x = 0; x < jpeg->w / 8 + !!(jpeg->w & 0x7); ++x) {
			idct matL, matCr, matCb;

			build_matrix(jpeg, &matL,  &bs, 0, jpeg->quant[jpeg->quant_mappings[0]->qtb_id], old_lum, &old_lum);
			build_matrix(jpeg, &matCb, &bs, 1, jpeg->quant[jpeg->quant_mappings[1]->qtb_id], old_cbd, &old_cbd);
			build_matrix(jpeg, &matCr, &bs, 1, jpeg->quant[jpeg->quant_mappings[2]->qtb_id], old_crd, &old_crd);

			draw_matrix(jpeg, x, y, &matL, &matCb, &matCr);
		}
	}
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
	fclose(f);

	jpeg_t* result = jpeg_decode(data, st.st_size);

	return result;
}

void jpeg_close(jpeg_t* file) {
	free(file->data);
	free(file);
}

jpeg_t* jpeg_decode(void* data, size_t size) {
	off_t offset = 0;
	jpeg_t* jpeg            = malloc(sizeof(jpeg_t));
	__jpeg_internal* __jpeg = calloc(1, sizeof(__jpeg_internal));
	while(offset < size) {
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

			printf(" +%d ", length);

 			if (header == 0xffdb) { //quant table
				printf(" -- QUANT TABLE\n");
				parse_quant_tables(__jpeg, data + offset, length);
			} else if (header == 0xffc0) { // dct
				printf(" -- DCT\n");
				parse_dct(__jpeg, data + offset, length);
			} else if (header == 0xffc4) { // hufman
				printf(" -- HUFFMAN\n");
				parse_huffman(__jpeg, data + offset, length);
			} else if (header == 0xffda) { // data
				printf(" -- DATA\n");
				parse_data(__jpeg, data + offset, length, size - offset);
				break;
			} else {
				printf(" -- UNKNOWN\n");
			}

			offset += length;
		}
	}

	jpeg->w = __jpeg->w;
	jpeg->h = __jpeg->h;
	jpeg->data = __jpeg->data;

	for(int i = 0; i < 3; i++) {
		if(!__jpeg->quant_mappings[i]) {
			continue;
		}
		free(__jpeg->quant_mappings[i]);
	}
	free(__jpeg);

	return jpeg;
}
