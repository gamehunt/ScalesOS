#include <string.h>

void *memset(void *dest, int c, size_t n) {
    unsigned char *p = dest;
    while (n > 0) {
        *p = c;
        p++;
        n--;
    }
    return (dest);
}