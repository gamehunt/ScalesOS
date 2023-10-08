#include <stdlib.h>
#include <sys/syscall.h>

extern void (*__atexit[__ATEXIT_MAX])(void);

void exit(int code){
	for(int i = 0; i < __ATEXIT_MAX; i++) {
		if(__atexit[i]) {
			__atexit[i]();
		}
	}
   __sys_exit(code);
   __builtin_unreachable();
}
