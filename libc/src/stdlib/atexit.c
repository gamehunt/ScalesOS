#include <stdlib.h>

void (*__atexit[__ATEXIT_MAX])(void) = {NULL};
int    __atexit_count = 0;

int atexit(void(*func)(void)){
	if(__atexit_count >= __ATEXIT_MAX) {
		return 1;
	}
	__atexit[__atexit_count] = func;
	__atexit_count++;
    return 0;
}
