#ifndef __SYS_SIGNAL_H
#define __SYS_SIGNAL_H

#include <sys/types.h>

#define MAX_SIGNAL 64

#define SIGKILL 9
#define SIGSEGV 11

typedef void (*_signal_handler_ptr)(int);

int kill(pid_t pid, int sig);

#endif