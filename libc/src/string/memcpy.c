#include <stdio.h>
#include <string.h>

void *memcpy(void *dest, const void *src, size_t n){
#ifdef HAS_ARCH_MEMSET
	extern void* __arch_memcpy(void*, void*, size_t);
	return __arch_memcpy(dest, src, n);
#else
    char *pszDest = (char *)dest;
    const char *pszSource = (const char*)src;
    if((pszDest!= NULL) && (pszSource != NULL))
    {
        while(n)
        {
            *(pszDest++)= *(pszSource++);
            --n;
        }
    }
    return dest;
#endif
}
