#include <errno.h>
#include <sys/syscall.h>

extern void __mem_place_heap(void* addr);

int setheap(void* addr) {
	int r = __sys_setheap((uint32_t) addr);
	if(r < 0) {
		__set_errno(-r);
		return -1;
	}

	__mem_place_heap(addr);
	return 0;
}
