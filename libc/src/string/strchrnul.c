#include <string.h>

char *strchrnul(const char *s, int c) {
    while (*s) {
        if (c == *s)
            break;
        s++;
    }
    return (char *)(s);
}