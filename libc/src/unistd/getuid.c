#include <unistd.h>
#include <sys/syscall.h>

uid_t getuid(void) {
	return __sys_getuid();
}

uid_t geteuid(void) {
	return __sys_geteuid();
}
