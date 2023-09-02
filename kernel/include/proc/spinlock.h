#ifndef __K_PROC_SPINLOCK_H
#define __K_PROC_SPINLOCK_H

typedef volatile int spinlock_t;

#if defined(__KERNEL) || defined(__LIBK)

extern void k_proc_process_yield();

#define YIELD() k_proc_process_yield() 

#else

#define YIELD() __sys_yield()

#endif

#define LOCK(spinlock) \
	while(__sync_lock_test_and_set(&spinlock, 1)){ \
		YIELD(); \
	}

#define UNLOCK(spinlock) \
	__sync_lock_release(&spinlock);

#endif
