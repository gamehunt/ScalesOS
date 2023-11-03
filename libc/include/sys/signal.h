#ifndef __SYS_SIGNAL_H
#define __SYS_SIGNAL_H

#include <sys/types.h>

#define MAX_SIGNAL 64

#define SIGHUP	  1	 // Hangup
#define SIGINT	  2	 // Interrupt
#define SIGQUIT	  3	 // Quit
#define SIGILL	  4	 // Illegal instruction
#define SIGTRAP	  5	 // Trace/breakpoint trap
#define SIGABRT	  6	 // Aborted
#define SIGIOT	  6	 // (Same value as SIGABRT) Aborted
#define SIGBUS	  7	 // Bus error
#define SIGFPE	  8	 // Floating point exception
#define SIGKILL	  9	 // Killed
#define SIGUSR1	  10 // User defined signal 1
#define SIGSEGV	  11 // Segmentation fault
#define SIGUSR2	  12 // User defined signal 2
#define SIGPIPE	  13 // Broken pipe
#define SIGALRM	  14 // Alarm clock
#define SIGTERM	  15 // Terminated
#define SIGSTKFLT 16 // Stack fault
#define SIGCHLD	  17 // Child exited
#define SIGCLD	  17 // (Same value as SIGCHLD) Child exited
#define SIGCONT	  18 // Continued
#define SIGSTOP	  19 // Stopped (signal)
#define SIGTSTP	  20 // Stopped
#define SIGTTIN	  21 // Stopped (tty input)
#define SIGTTOU	  22 // Stopped (tty output)
#define SIGURG	  23 // Urgent I/O condition
#define SIGXCPU	  24 // CPU time limit exceeded
#define SIGXFSZ	  25 // File size limit exceeded
#define SIGVTALRM 26 //	Virtual timer expired
#define SIGPROF	  27 // Profiling timer expired
#define SIGWINCH  28 //	Window changed
#define SIGPOLL	  29 // I/O possible
#define SIGIO	  29 // (Same value as SIGPOLL) I/O possible
#define SIGPWR	  30 // Power failure
#define SIGSYS	  31 // Bad system call

typedef void (*_signal_handler_ptr)(int);

int kill(pid_t pid, int sig);

#endif
