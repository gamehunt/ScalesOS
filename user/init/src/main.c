#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

void init_output() {
	__sys_open("/dev/console", 0, 0);
	__sys_open("/dev/console", 1, 0);
	__sys_open("/dev/console", 1, 0);
}

void test_handler(int a) {
	printf("Signal received!\r\n");
	sleep(1);
}

int main(int argc, char** argv){
	init_output();

	printf("Hello, world! %ld\r\n", time(0));

	pid_t child = fork();
	if(!child) {
		signal(0, test_handler);
		for(int i = 0; i < 255; i++){
			printf("%d\r\n", i);
		}
	} else {
		sleep(2);
		printf("Sending signal...\r\n");
		kill(child, 0);
	}

	while(1);

	__sys_reboot();
	__builtin_unreachable();
}
