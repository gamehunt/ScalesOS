#ifndef __SCALES_MMAP
#define __SCALES_MMAP

#include <sys/types.h>

typedef struct {
	int   fd;
	off_t offset;
} file_arg_t;

#endif
