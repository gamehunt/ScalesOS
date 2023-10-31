#ifndef __K_MEM_SHM_H
#define __K_MEM_SHM_H

#ifdef __KERNEL
#include "fs/vfs.h"
#else
#include "kernel/fs/vfs.h"
#endif

#include "types/list.h"

typedef struct {
	char        name[256];
	list_t* 	frames;
	int     	links;
	fs_node_t*  root;
} shm_node_t;

void    k_mem_shm_init();
uint8_t k_mem_is_shm(fs_node_t* node);

#endif
