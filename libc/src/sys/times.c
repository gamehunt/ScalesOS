#include "sys/times.h"
#include "sys/syscall.h"

clock_t times(struct tms* tms) {
	return __sys_times((uint32_t) tms);
}
