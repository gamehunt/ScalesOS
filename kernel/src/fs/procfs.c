#include "fs/procfs.h"
#include "dirent.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/process.h"
#include <stdio.h>
#include <string.h>

static struct dirent* __k_fs_procfs_readdir(fs_node_t* node UNUSED, uint32_t index) {
	if (index == 0) {
		struct dirent * out = k_malloc(sizeof(struct dirent));
		memset(out, 0x00, sizeof(struct dirent));
		out->ino = 0;
		strcpy(out->name, ".");
		return out;
	}

	if (index == 1) {
		struct dirent * out = malloc(sizeof(struct dirent));
		memset(out, 0x00, sizeof(struct dirent));
		out->ino = 0;
		strcpy(out->name, "..");
		return out;
	}

	if (index == 2) {
		struct dirent * out = malloc(sizeof(struct dirent));
		memset(out, 0x00, sizeof(struct dirent));
		out->ino = 0;
		strcpy(out->name, "self");
		return out;
	}

	index -= 3;

	list_t* processes = k_proc_process_list();

	if(index < processes->size) {
		process_t* process = processes->data[index];
		struct dirent * out = k_malloc(sizeof(struct dirent));
		memset(out, 0x00, sizeof(struct dirent));
		out->ino = 0;
		snprintf(out->name, sizeof(out->name), "%d", process->pid);
		return out;
	}

	return NULL;
}

static fs_node_t* __k_fs_procfs_create_root() {
	fs_node_t* fsroot = k_fs_vfs_create_node("proc");
	fsroot->inode = 1;
	fsroot->flags = VFS_DIR;
	fsroot->fs.readdir = &__k_fs_procfs_readdir;
	return fsroot;
}

int k_fs_procfs_init() {
	return k_fs_vfs_mount_node("/proc", __k_fs_procfs_create_root());
}
