#include "sys/utsname.h"
#include "sys/syscall.h"
#include "errno.h"

int uname(struct utsname *buf) {
	__return_errno(__sys_uname(buf));
} 
