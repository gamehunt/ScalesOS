#include <stdlib.h>
#include <stddef.h>

extern void _free(void*);

void free(void* mem){
    _free(mem);
}