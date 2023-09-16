#include "sys/mod.h"
#include "sys/syscall.h"

int insmod(void* buffer, size_t size) {
	return __sys_insmod((uint32_t) buffer, size);
}

