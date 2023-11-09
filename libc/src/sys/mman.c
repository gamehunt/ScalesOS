#include "sys/mman.h"
#include "unistd.h"
#include <stdio.h>

int shm_open(const char *name, int oflag, mode_t mode) {
	char buff[255];
	snprintf(buff, sizeof(buff), "/dev/shm/%s", name);
	return open(buff, oflag);
}

int shm_unlink(const char *name) {
	char buff[255];
	snprintf(buff, sizeof(buff), "/dev/shm/%s", name);
	return remove(name);
}
