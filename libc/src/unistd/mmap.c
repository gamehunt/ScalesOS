#include "errno.h"
#include "sys/mman.h"
#include "scales/mmap.h"
#include "sys/syscall.h"
#include <unistd.h>

void* mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset) {
	file_arg_t file;
	file.fd     = fd;
	file.offset = offset;
	long value = __sys_mmap((uint32_t) start, length, prot, flags, (uint32_t) &file);
	if(value < 0) {
		__set_errno(-value);
		return (void*) MAP_FAILED;
	}
	return (void*) value;
}

int munmap(void *start, size_t length) {
	__return_errno(__sys_munmap((uint32_t) start, length));
}

int msync(void *start, size_t length, int flags) {
	return -1;
}
