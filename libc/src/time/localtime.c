#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct tm __result;

static char* current_timezone() {
	char* tz = getenv("TZ");
	return tz ? tz : "???";
}

static int current_timezone_offset() {
	char * tzOff = getenv("TZ_OFFSET");
	if (!tzOff) return 0;
	char * endptr;
	int out = strtol(tzOff, &endptr, 10);
	if (*endptr) return 0;
	return out;
}

static void __make_result(const time_t* time, const char* tz, int tzOffset) {
	time_t msec = *time + tzOffset;

	__result._tm_timezone_name   = tz;
	__result._tm_timezone_offset = tzOffset;

	// TODO
}

struct tm* gmtime(const time_t* time) {
	__make_result(time, "UTC", 0);
	return &__result;
}

struct tm* localtime(time_t* time) {
	__make_result(time, current_timezone(), current_timezone_offset());	
	return &__result;
}
