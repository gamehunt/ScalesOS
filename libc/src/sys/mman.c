#include "sys/mman.h"
#include "unistd.h"
#include "stdio.h"

int shm_open(const char *name, int oflag, mode_t mode) {
	return open(name, oflag);
}

int shm_unlink(const char *name) {
	return remove(name);
}
