#include "fcntl.h"
#include "errno.h"
#include "sys/syscall.h"

int fcntl(int fd, int cmd, long arg) {
	__return_errno(__sys_fcntl(fd, cmd, arg));
}

