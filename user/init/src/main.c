#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

void init_output() {
	__sys_open("/dev/console", O_RDONLY, 0); // stdin
	__sys_open("/dev/console", O_WRONLY, 0); // stdout
	__sys_open("/dev/console", O_WRONLY, 0); // stderr
}

void test_handler(int a) {
	printf("Signal received!\r\n");
	sleep(1);
}

int main(int argc, char** argv){
	init_output();

	char* kernel = getenv("KERNEL");
	if(!kernel) {
		kernel = "UNKNOWN";
	}

	printf("Hello, world! %ld %s\r\n", time(0), kernel);

	pid_t child = fork();
	if(!child) {
		signal(0, test_handler);
	} else {
		sleep(2);
		printf("Sending signal...\r\n");
		kill(child, SIGKILL);
	}

	while(1);

	__sys_reboot();
	__builtin_unreachable();
}
