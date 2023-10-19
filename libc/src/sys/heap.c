#include <errno.h>
#include <sys/syscall.h>
#include <sys/heap.h>

extern void __mem_place_heap(void* addr);

static heap_opts_t __heapopts = HEAP_OPT_DEFAULTS;

int setheap(void* addr) {
	int r = __sys_setheap((uint32_t) addr);
	if(r < 0) {
		__set_errno(-r);
		return -1;
	}

	__mem_place_heap(addr);
	return 0;
}

void setheapopts(heap_opts_t opt) {
	__heapopts = opt;
}

heap_opts_t getheapopts() {
	return __heapopts;
}
