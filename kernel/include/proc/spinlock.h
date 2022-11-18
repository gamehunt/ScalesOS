#ifndef __K_PROC_SPINLOCK_H
#define __K_PROC_SPINLOCK_H

typedef volatile int spinlock_t;

#define LOCK(spinlock) \
    while(!__sync_bool_compare_and_swap(&spinlock, 0, 1)){ asm("pause"); }

#define UNLOCK(spinlock) \
    spinlock = 0;

#endif