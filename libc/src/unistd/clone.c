#include "scales/sched.h"
#include "sys/syscall.h"
#include "errno.h"

int clone(clone_args_t* args) {
	__return_errno(__sys_clone((uint32_t) args));
}
