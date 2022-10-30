#include <stdlib.h>
#include <stddef.h>

extern void* _malloc(size_t);

void* malloc(size_t size){
    return _malloc(size);
}