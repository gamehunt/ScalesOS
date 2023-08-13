#include <stdlib.h>

#ifdef __LIBK
extern void k_panic(const char* reason, void* ctx);
#endif

void  abort(){
#ifdef __LIBK
    k_panic("Abort() called.", 0);
#else
	exit(3);
#endif
}
