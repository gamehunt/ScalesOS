#include <stdlib.h>
#include <stddef.h>

#ifdef __LIBK
extern void _free(void*);
#else
#ifdef __LIBC
void _free(void* ptr) {

}
#endif
#endif

void free(void* mem){
    _free(mem);
}
