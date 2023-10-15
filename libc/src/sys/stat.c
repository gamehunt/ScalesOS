#include "sys/stat.h"
#include "errno.h"
#include "sys/syscall.h"

int mkfifo(const char* path, int mode) {
	return __sys_mkfifo((uint32_t) path, mode);
}

int stat(const char *file, struct stat *st) {
	__return_errno(__sys_stat((uint32_t) file, (uint32_t) st));
}

int lstat(const char *path, struct stat *st) {
	__return_errno(__sys_lstat((uint32_t) path, (uint32_t) st));
}

int fstat(int fd, struct stat *st) {
	__return_errno(__sys_fstat((uint32_t) fd, (uint32_t) st));
}

int mkdir(const char *pathname, mode_t mode) {
	__return_errno(__sys_mkdir((uint32_t) pathname, mode));
}

mode_t umask(mode_t mask) {

}
