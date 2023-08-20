#include "sys/syscall.h"
#include <signal.h>

signal_handler_t signal(int sig, signal_handler_t handler) {
	return (signal_handler_t) __sys_signal(sig, (uint32_t) handler);
}
