#include <signal.h>
#include <stdlib.h>

#ifdef __LIBK
extern void k_panic(const char* reason, void* ctx);
#endif

void  abort(){
#ifdef __LIBK
    k_panic("abort() called.", 0);
#else
	raise(SIGABRT);
#endif
}
