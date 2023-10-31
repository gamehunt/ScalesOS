#include "pthread.h"
#include "scales/sched.h"
#include "sys/syscall.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

static void __start_thread(pthread_t thread) {
	pthread_exit(thread->entry(thread->arg));
}

int pthread_create(pthread_t* thread, const pthread_attr_t attr,
                          void* (*start_routine)(void *),
						  void* arg) {
	void* stack = valloc(PTHREAD_STACK_SIZE, 4);
	memset(stack, 0, PTHREAD_STACK_SIZE);

	__pthread_t* thr = malloc(sizeof(__pthread_t));
	thr->arg   = arg;
	thr->entry = start_routine;

	*thread = thr;

	clone_args_t args;
	args.stack = (uint32_t) (stack);
	args.stack_size = PTHREAD_STACK_SIZE;
	args.flags = CLONE_THREAD | CLONE_VM | CLONE_FILES | CLONE_FS;
	args.entry = (void*) __start_thread;
	args.arg   = (void*) thr;

	clone(&args);

	return 0;
}

int pthread_join(pthread_t thread, void **retval) {
	waitpid(thread->tid, 0, NULL);
	*retval = thread->ret;
	return 0;
}

void pthread_exit(void* value) {
	__sys_exit(0);
}
