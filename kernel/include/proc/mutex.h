#ifndef __K_PROC_MUTEX
#define __K_PROC_MUTEX

#include "proc/process.h"
#include "proc/spinlock.h"
#include "types/list.h"

#include <stddef.h>

typedef struct {
	spinlock_t lock;
	uint8_t    locked;
	process_t* owner;
	list_t*    wait_queue;
} mutex_t;

void mutex_init(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

#endif
