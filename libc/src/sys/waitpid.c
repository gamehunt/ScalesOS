#include "sys/syscall.h"
#include <sys/wait.h>
#include <errno.h>

pid_t waitpid(pid_t pid, int *status, int options) {
	__return_errno(__sys_waitpid(pid, (uint32_t) status, options));
}

