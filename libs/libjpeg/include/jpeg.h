#ifndef __LIB_JPEG_H
#define __LIB_JPEG_H

#include <stddef.h>
#include <stdint.h>

typedef struct {

} jpeg_t;

jpeg_t* jpeg_open(const char* path);
void    jpeg_close(jpeg_t* file);
jpeg_t* jpeg_decode(void* data, size_t size);

#endif
