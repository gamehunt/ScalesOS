#include "scales/memory.h"

extern int __memset16(void* dest, uint16_t v, size_t n);
extern int __memset32(void* dest, uint32_t v, size_t n);


int memset16(void* dest, uint16_t v, size_t n) {
	return __memset16(dest, v, n);
}

int memset32(void* dest, uint32_t v, size_t n) {
	return __memset32(dest, v, n);
}
