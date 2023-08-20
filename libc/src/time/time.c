#include "sys/time.h"
#include <time.h>

time_t time(time_t* tm) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	if(tm) {
		*tm = tv.tv_sec;
	}
	return tv.tv_sec;
}
