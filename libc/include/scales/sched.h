#ifndef __SCALES_SCHED_H
#define __SCALES_SCHED_H

#include <sys/types.h>
#include <stdint.h>

typedef struct clone_args {
    uint32_t flags;      
    int*     pidfd;      
    pid_t*   child_tid;  
    pid_t*   parent_tid; 
    uint32_t exit_signal;
    uint32_t stack;      
    uint32_t stack_size;  
    uint32_t tls;         
	void*    (*entry) (void*);
	void*    (*arg)   (void*);
} clone_args_t;

#endif
