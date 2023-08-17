#include "sys/syscall.h"
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int options) {
	return __sys_waitpid(pid, (uint32_t) status, options);
}

