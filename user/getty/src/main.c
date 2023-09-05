#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
	FILE* tty = fopen("/dev/tty0", "r+");
	if(!tty) {
		return 1;
	}
	
	dup2(fileno(tty), STDIN_FILENO);
	dup2(fileno(tty), STDOUT_FILENO);
	dup2(fileno(tty), STDERR_FILENO);

	system("stty sane");

	execve("/bin/login", NULL, NULL);
	return 1;
}
