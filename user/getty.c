#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/tty.h>

static void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 0;
	}

	int number = argv[1][0] - '0';
	char path[32];
	snprintf(path, 32, "/dev/tty%d", number);

	FILE* tty = fopen(path, "r+");
	if(!tty) {
		return 2;
	}

	dup2(fileno(tty), STDIN_FILENO);
	dup2(fileno(tty), STDOUT_FILENO);
	dup2(fileno(tty), STDERR_FILENO);

	system("stty sane");

	printf("ScalesOS V1.0.0 | %s\n\n", path);

	execve("/bin/login", NULL, NULL);
	return 1;
}
