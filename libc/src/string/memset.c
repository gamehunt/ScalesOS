#include <stdio.h>
#include <string.h>

void *memset(void *dest, int c, size_t n) {
#ifdef HAS_ARCH_MEMSET
	extern void* __arch_memset(void*, int, size_t);
	return __arch_memset(dest, c, n);
#else
    unsigned char *p = dest;
    while (n > 0) {
        *p = c;
        p++;
        n--;
    }
    return (dest);
#endif
}
