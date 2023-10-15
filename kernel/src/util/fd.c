#include "util/fd.h"
#include <stddef.h>

fd_t* fd2fdt(fd_list_t* fds, int fd) {
	if(!fds || fd < 0 || fds->size <= fd || !fds->nodes[fd]) {
		return NULL;
	}

	return fds->nodes[fd];
}
