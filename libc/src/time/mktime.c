#include <time.h>

#define SEC_DAY 86400

static int year_is_leap(int year) {
	return ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)));
}

static long days_in_month(int month, int year) {
	switch(month) {
		case 12:
			return 31;
		case 11:
			return 30;
		case 10:
			return 31;
		case 9:
			return 30;
		case 8:
			return 31;
		case 7:
			return 31;
		case 6:
			return 30;
		case 5:
			return 31;
		case 4:
			return 30;
		case 3:
			return 31;
		case 2:
			return year_is_leap(year) ? 29 : 28;
		case 1:
			return 31;
	}
	return 0;
}
static unsigned int secs_of_years(int years) {
	unsigned int days = 0;
	while (years > 1969) {
		days += 365;
		if (year_is_leap(years)) {
			days++;
		}
		years--;
	}
	return days * 86400;
}

static long secs_of_month(int months, int year) {
	long days = 0;
	for (int i = 1; i < months; ++i) {
		days += days_in_month(i, year);
	}
	return days * SEC_DAY;
}

time_t mktime(struct tm *tm) {
	return
	  secs_of_years(tm->tm_year + 1899) *  +
	  secs_of_month(tm->tm_mon + 1, tm->tm_year + 1900) +
	  (tm->tm_mday - 1) * 86400 +
	  (tm->tm_hour) * 3600 +
	  (tm->tm_min) * 60 +
	  (tm->tm_sec);
}
