#include "sys/syscall.h"
#include <sys/signal.h>

int kill(pid_t pid, int sig) {
	return __sys_kill(pid, sig);
}
