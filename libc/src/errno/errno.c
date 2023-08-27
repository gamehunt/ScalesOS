#include <errno.h>

static int __errno = 0;

void __set_errno(long e) {
	__errno = e;
}

long __get_errno() {
	return __errno;
}
