#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

extern void __mem_init_heap();

void libc_init() {
	__mem_init_heap();
}

void libc_exit(int code) {
	exit(code);
}
