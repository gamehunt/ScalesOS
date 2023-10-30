#include "proc/mutex.h"
#include "proc/process.h"
#include "types/list.h"

void mutex_init(mutex_t* mutex) {
	mutex->lock       = 0;
	mutex->owner      = NULL;
	mutex->wait_queue = list_create();
}

void mutex_lock(mutex_t* mutex) {
	LOCK(mutex->lock)

	while(mutex->locked) {
		process_t* cur = k_proc_process_current();
		k_proc_process_sleep_and_unlock(cur, mutex->wait_queue, &mutex->lock);		
	}	
	
	mutex->locked = 1;
	mutex->owner  = k_proc_process_current();

	UNLOCK(mutex->lock)
}

void mutex_unlock(mutex_t* mutex) {
	LOCK(mutex->lock)

	mutex->locked = 0;
	mutex->owner  = NULL;

	k_proc_process_wakeup_queue_single(mutex->wait_queue);

	UNLOCK(mutex->lock)
}
