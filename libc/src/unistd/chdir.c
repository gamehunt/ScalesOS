#include "errno.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/syscall.h>

int chdir(const char *path) {
	FILE* dir = fopen(path, "r");
	if(!dir) {
		__return_errno(-EINVAL);
	}
	return fchdir(fileno(dir));
}

int fchdir(int fd) {
	__return_errno(__sys_chdir(fd));
} 
