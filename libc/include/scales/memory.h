#ifndef __SCALES_MEMORY_H
#define __SCALES_MEMORY_H

#include <stddef.h>
#include <stdint.h>

int memset16(void* dest, uint16_t v, size_t n);
int memset32(void* dest, uint32_t v, size_t n);

#endif
