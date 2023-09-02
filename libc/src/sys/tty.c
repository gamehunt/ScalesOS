#include "sys/tty.h"
#include "errno.h"

int openpty(int* master, int* slave, char* name, struct termios* params, struct winsize* ws) {
	 __return_errno(__sys_openpty(master, slave, name, params, ws));
}
