#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "sys/signal.h"

#define SIG_DFL ((_signal_handler_ptr)0) /* Default action */
#define SIG_IGN ((_signal_handler_ptr)1) /* Ignore action */
#define SIG_ERR ((_signal_handler_ptr)-1)/* Error return */

typedef _signal_handler_ptr signal_handler_t;
typedef int sig_atomic_t;

signal_handler_t signal(int sig, signal_handler_t handler);
int  raise(int sig);

#endif
