#include <string.h>

void *memcpy (void *dest, const void *src, size_t n){
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
}