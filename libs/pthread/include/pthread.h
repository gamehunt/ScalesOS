#ifndef __LIB_PTHREAD_H
#define __LIB_PTHREAD_H

#include "kernel/types.h"
#include <stdint.h>

#define PTHREAD_STACK_SIZE 8192

typedef struct {
	pid_t tid;
	void* arg;
	void* (*entry)(void*);
	void* ret;
} __pthread_t;

typedef __pthread_t* pthread_t;
typedef uint32_t pthread_attr_t;

int  pthread_create(pthread_t* thread, const pthread_attr_t attr,
                          void* (*start_routine)(void *),
                          void* arg);
int  pthread_join(pthread_t thread, void **retval);
void pthread_exit(void* value);

#endif
