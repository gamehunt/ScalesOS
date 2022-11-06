#include <stdlib.h>
#include <stddef.h>

#ifdef __LIBK
extern void* _malloc(size_t);
#endif

void* malloc(size_t size){
#ifdef __LIBK
    return _malloc(size);
#endif
}