#ifndef __K_UTIL_RINGBUFFER
#define __K_UTIL_RINGBUFFER

#ifdef __KERNEL
#include "proc/spinlock.h"
#else
#include "kernel/proc/spinlock.h"
#endif

#include <stdint.h>
#include "types/list.h"

typedef struct ringbuffer {
	uint8_t* buffer;
	uint32_t size;
	uint32_t read_ptr;
	uint32_t write_ptr;
	spinlock_t lock;
	list_t* read_wait_queue;
	list_t* write_wait_queue;
} ringbuffer_t;

ringbuffer_t* ringbuffer_create(uint32_t size);
void          ringbuffer_free(ringbuffer_t* rb);
uint32_t ringbuffer_read_available(ringbuffer_t* rb);
uint32_t ringbuffer_write_available(ringbuffer_t* rb);
uint32_t ringbuffer_write(ringbuffer_t* rb, uint32_t size, uint8_t* buffer);
uint32_t ringbuffer_read(ringbuffer_t* rb, uint32_t size, uint8_t* buffer);

#endif
