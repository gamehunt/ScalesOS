#ifndef __SYS_TIMES
#define __SYS_TIMES

#include <time.h>

struct tms {
	clock_t tms_utime;
	clock_t tms_stime;
};

clock_t times(struct tms* tms);

#endif
