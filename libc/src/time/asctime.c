#include <time.h>

static char output[26];
char* asctime(const struct tm *tm) {
	strftime(output, 26, "%a %b %d %T %Y\n", tm);
	return output;
}
