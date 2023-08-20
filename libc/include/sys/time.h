#ifndef __SYS_TIME_H
#define __SYS_TIME_H

#include <sys/types.h>

struct timeval {
	time_t tv_sec;
	msec_t tv_msec;
};

struct timezone {
	char TODO;
};

int gettimeofday(struct timeval*, struct timezone*);
int settimeofday(struct timeval*, struct timezone*);

#endif
