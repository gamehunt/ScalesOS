#include "sys/times.h"
#include <time.h>

clock_t clock(void) {
	struct tms ts;
	times(&ts);
	return ts.tms_utime;
}
