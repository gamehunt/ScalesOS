#include "sys/mod.h"
#include "sys/syscall.h"

int insmod(void* buffer) {
	return __sys_insmod((uint32_t) buffer);
}

