#include "errno.h"
#include "sys/syscall.h"
#include "unistd.h"

size_t read(int fd, void *buf, size_t count) {
	__return_errno(__sys_read(fd, count, (uint32_t) buf));
}

size_t write(int fd, void *buf, size_t count) {
	__return_errno(__sys_write(fd, count, (uint32_t) buf));
}

int open(const char *pathname, int flags) {
	__return_errno(__sys_open(pathname, flags, 0));
}

int close(int fd) {
	__return_errno(__sys_close(fd));
}
