#include <stdlib.h>
#include <sys/syscall.h>

extern void (*__atexit)(void);

void exit(int code){
	if(__atexit) {
		__atexit();
	}
   __sys_exit(code);
   __builtin_unreachable();
}
