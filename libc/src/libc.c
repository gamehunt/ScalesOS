#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>


void libc_init() {
	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 0, 0);
}

void libc_exit(int code) {
	exit(code);
}
