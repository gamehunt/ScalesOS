#ifndef __TIME_H
#define __TIME_H

#include <stddef.h>
#include <sys/types.h>

#define CLOCKS_PER_SEC 1000000

struct tm {
	int tm_sec;		
	int tm_min;	
	int tm_hour;
	int tm_mday;
	int tm_mon;	
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;

	const char* _tm_timezone_name;
	int  		_tm_timezone_offset;
};

clock_t     clock(void);
time_t      time(time_t *tp);
char*       ctime(time_t timp);
double      difftime(time_t time2,time_t time1);
time_t      mktime(struct tm *tp);
char*       asctime(const struct tm *tp);
struct tm*  gmtime(const time_t *timep);
struct tm*  localtime(time_t *timep);
size_t      strftime(char *s, size_t maxsize, const char *format, const struct tm *timp);

#endif
