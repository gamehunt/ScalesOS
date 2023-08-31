#include "unistd.h"
#include "sys/syscall.h"

int dup(int oldfd) {
	return __sys_dup2(oldfd, -1);
}

int dup2(int oldfd, int newfd) {
	return __sys_dup2(oldfd, newfd);
}
