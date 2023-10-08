#include "sys/syscall.h"
#include <sys/reboot.h>

int reboot(int op) {
	return __sys_reboot(op);
}
