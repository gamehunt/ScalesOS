#include <stdlib.h>
#include <stddef.h>

#ifdef __LIBK
extern void _free(void*);
#endif

void free(void* mem){
#ifdef __LIBK
    _free(mem);
#endif
}