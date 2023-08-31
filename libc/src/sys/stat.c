#include "sys/stat.h"
#include "sys/syscall.h"

int mkfifo(const char* path, int mode) {
	return __sys_mkfifo((uint32_t) path, mode);
}

