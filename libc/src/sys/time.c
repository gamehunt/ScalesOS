#include "sys/syscall.h"
#include <sys/time.h>

int gettimeofday(struct timeval* tv, struct timezone* tz) {
	return __sys_gettimeofday((uint32_t) tv, (uint32_t) tz);
}

int settimeofday(struct timeval* tv, struct timezone* tz) {
	return __sys_settimeofday((uint32_t) tv, (uint32_t) tz);
}
