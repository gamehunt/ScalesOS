#include "errno.h"
#include "sys/syscall.h"
#include <unistd.h>
#include <stddef.h>

char* getcwd(char buf[], size_t size) {
	int r = __sys_getcwd((uint32_t) buf, size);

	if(r < 0) {
		__set_errno(-r);
		return NULL;
	}

	return buf;
}

char* get_current_dir_name(void) {
	static char wd[255];
	return getcwd(wd, sizeof(wd));
}
