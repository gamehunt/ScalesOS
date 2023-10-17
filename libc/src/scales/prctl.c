#include "errno.h"
#include "sys/syscall.h"
#include <scales/prctl.h>

int prctl(int op, void* arg) {
	__return_errno(__sys_prctl(op, (uint32_t) arg));
}
