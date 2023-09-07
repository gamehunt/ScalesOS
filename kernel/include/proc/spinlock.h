#ifndef __K_PROC_SPINLOCK_H
#define __K_PROC_SPINLOCK_H

typedef volatile int spinlock_t;

#if defined(__KERNEL) || defined(__LIBK)

extern void k_proc_process_yield();

#define YIELD() k_proc_process_yield() 
#define PERF_WARN(lock, cycles) \
	cycles++; \
	if(cycles > 100) { \
		k_panic(#lock " hangup", 0); \
	} 


#else

#define YIELD() __sys_yield()
#define PERF_WARN(lock, cycles)

#endif

#define LOCK(spinlock) \
	uint32_t cycles = 0; \
	while(__sync_lock_test_and_set(&spinlock, 1)){ \
		PERF_WARN(spinlock, cycles); \
		YIELD(); \
	}

#define UNLOCK(spinlock) \
	__sync_lock_release(&spinlock);

#endif
