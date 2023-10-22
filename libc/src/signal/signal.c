#include "sys/syscall.h"
#include <signal.h>
#include <stddef.h>
#include <errno.h>

int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
	__return_errno(__sys_sigact(signum, act, oldact));
}

signal_handler_t signal(int sig, signal_handler_t handler) {
	struct sigaction act;
	act.sa_handler   = handler;
	act.sa_sigaction = NULL;
	act.sa_mask      = 0;
	act.sa_flags     = 0;

	struct sigaction oldact;

	int r = sigaction(sig, &act, &oldact);
	if(r < 0) {
		return (signal_handler_t) r;
	}

	return oldact.sa_handler;
}
