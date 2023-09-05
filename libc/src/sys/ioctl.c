#include "sys/ioctl.h"
#include "errno.h"
#include "sys/syscall.h"
#include <stdarg.h>

int ioctl(int fd, int req, void* args) {
	__return_errno(__sys_ioctl(fd, req, args));
}
