#include "dev/vt.h"
#include "proc/process.h"
#include "util/log.h"
#include <stddef.h>

void test_tasklet(uint32_t data) {
	k_debug("0x%.8x", (uintptr_t) &data);
	while(1) {;}
}

int k_dev_vt_init() {
	k_proc_process_create_tasklet("[vt_test]", (uintptr_t) &test_tasklet, (void*) 0xAABBCCDD);
	return 0;
}
