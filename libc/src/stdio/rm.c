#include <stdio.h>
#include <errno.h>
#include <sys/syscall.h>

int remove(const char* path) {
	return unlink(path);
}

int unlink(const char* path) {
	__return_errno(__sys_rm(path));
}

int rmdir(const char* path) {
	__return_errno(__sys_rmdir(path));
}
