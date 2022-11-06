#include <stdlib.h>
#include <string.h>

#ifdef __LIBK
extern void* _malloc(size_t);
#endif

void *calloc(size_t num, size_t size){
#ifdef __LIBK
    void* mem = _malloc(num * size);
#endif
    memset(mem, 0, num * size);
    return mem;
}