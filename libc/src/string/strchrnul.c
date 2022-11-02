#include <string.h>

char *strchrnul (const char *s, int c){
    while (*s != (char) c) {
        if (!*s++) {
            return (char*) s;
        }
    }
    return (char *)s;
}