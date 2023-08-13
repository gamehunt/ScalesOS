#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

extern void __mem_init_heap();

void libc_init() {
	__mem_init_heap();

	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 0, 0);
}

void libc_exit(int code) {
	exit(code);
}
