#include "util/types/ringbuffer.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "util/types/list.h"

ringbuffer_t* ringbuffer_create(uint32_t size) {
	ringbuffer_t* rb = k_calloc(1, sizeof(ringbuffer_t));
	rb->buffer = k_malloc(size);
	rb->size = size;
	rb->read_wait_queue = list_create();
	rb->write_wait_queue = list_create();
	return rb;
}

void ringbuffer_free(ringbuffer_t* rb) {
	k_proc_process_wakeup_queue(rb->read_wait_queue);
	k_proc_process_wakeup_queue(rb->write_wait_queue);

	list_free(rb->read_wait_queue);
	list_free(rb->write_wait_queue);
	
	k_free(rb->buffer);
	k_free(rb);
}

uint32_t ringbuffer_read_available(ringbuffer_t* rb) {
	if(rb->read_ptr == rb->write_ptr) {
		return 0;
	}
	if(rb->read_ptr > rb->write_ptr) {
		return rb->size - rb->read_ptr + rb->write_ptr;
	} else {
		return rb->write_ptr - rb->read_ptr;
	}
}

uint32_t ringbuffer_write_available(ringbuffer_t* rb) {
	if(rb->read_ptr == rb->write_ptr) {
		return rb->size - 1;
	}
	if(rb->read_ptr > rb->write_ptr) {
		return rb->read_ptr - rb->write_ptr - 1;
	} else {
		return (rb->size - rb->write_ptr) + rb->read_ptr - 1;
	}
}

static void __ringbuffer_bump_write_ptr(ringbuffer_t* rb) {
	rb->write_ptr++;
	if(rb->write_ptr == rb->size) {
		rb->write_ptr = 0;
	}
}

static void __ringbuffer_bump_read_ptr(ringbuffer_t* rb) {
	rb->read_ptr++;
	if(rb->read_ptr == rb->size) {
		rb->read_ptr = 0;
	}
}

uint32_t ringbuffer_write(ringbuffer_t* rb, uint32_t size, uint8_t* buffer) {
	uint32_t written = 0;
	while(written < size) {
		while(ringbuffer_write_available(rb) && written < size) {
			rb->buffer[rb->write_ptr] = *buffer;
			written++;
			buffer++;
			__ringbuffer_bump_write_ptr(rb);
		}
		k_proc_process_wakeup_queue(rb->read_wait_queue);
		if(written < size) {
			k_proc_process_sleep_on_queue(k_proc_process_current(), rb->write_wait_queue);
		}
	}
	k_proc_process_wakeup_queue(rb->read_wait_queue);
	return written;
}

uint32_t ringbuffer_read(ringbuffer_t* rb, uint32_t size, uint8_t* buffer) {
	uint32_t read = 0;
	while(read < size) {
		while(ringbuffer_read_available(rb) && read < size) {
			*buffer = rb->buffer[rb->read_ptr];
			read++;
			buffer++;
			__ringbuffer_bump_read_ptr(rb);
		}
		k_proc_process_wakeup_queue(rb->write_wait_queue);
		if(read < size) {
			k_proc_process_sleep_on_queue(k_proc_process_current(), rb->read_wait_queue);
		}
	}
	k_proc_process_wakeup_queue(rb->write_wait_queue);
	return read;
}
