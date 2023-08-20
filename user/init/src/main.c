#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

void init_output() {
	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 1, 0);
	__sys_open("/dev/console", 1, 0);
}

int main(int argc, char** argv){
	init_output();

	printf("Hello, world!\r\n");

	__sys_reboot();
	__builtin_unreachable();
}
