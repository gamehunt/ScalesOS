#include "sys/syscall.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

int closedir(DIR* dir) {
	int result = __sys_close(dir->fd);
	free(dir);
	return result;
}

DIR* opendir(const char* path) {
	DIR* dir   = malloc(sizeof(DIR));
	dir->fd    = __sys_open((uint32_t) path, O_RDONLY, 0);
	dir->index = 0;
	return dir;
}

struct dirent *readdir(DIR* dir) {
	static struct dirent result;
	uint32_t res = __sys_readdir(dir->fd, ++dir->index, (uint32_t) &result);

	if(res < 0) {
		//SET ERRNO
		memset(&result, 0, sizeof(struct dirent));
		return 0;
	}

	if(res == 0) {
		memset(&result, 0, sizeof(struct dirent));
		return 0;
	}

	return &result;
}

int readdir_r(DIR *restrict dir, struct dirent *restrict entry, struct dirent **restrict result){
	fprintf(stderr, "readdir_r: UNIMPL");
	abort();
	return 0;
}

void rewinddir(DIR* dir) {
	dir->index = 0;
}

void seekdir(DIR* dir, long pos) {
	dir->index = pos;
}

long telldir(DIR* dir) {
	return dir->index;
}
