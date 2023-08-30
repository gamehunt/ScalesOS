#include "stdlib.h"
#include "sys/wait.h"
#include "types.h"
#include "unistd.h"

int system(const char* cmd) {
	char* args[] = {
		"-c",
		(char*) cmd,
		0
	};

	pid_t pid = fork();
	if(!pid) {
		execve("/bin/scsh", args, 0);
		exit(1);
	} else {
		int status;
		waitpid(pid, &status, 0);
		return status;
	}
}
