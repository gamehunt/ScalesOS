#include <unistd.h>
#include <sys/syscall.h>

unsigned int usleep(unsigned long usec) {
	return __sys_sleep(usec);
}
