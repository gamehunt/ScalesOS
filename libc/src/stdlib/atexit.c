#include <stdlib.h>

void (*__atexit)(void) = 0;

int atexit(void(*func)(void)){
	__atexit = func;
    return 0;
}
