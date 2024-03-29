#include "fs/procfs.h"
#include "dirent.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "util/log.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	const char* name;
	void (*fill)(fs_node_t* node);
} procdir_entry_t;

typedef struct {
	fs_node_t        node;
	char*            buffer;
	size_t           buffer_length;
	size_t           buffer_capacity;
	const procdir_entry_t* entry;
} proc_fs_node_t;

static void __k_fs_procfs_write_buffer(char c, void* n) {
	proc_fs_node_t* node = n;
	char* buffer = node->buffer;
	while(node->buffer_length >= node->buffer_capacity) {
		node->buffer_capacity += 64;
		node->buffer = realloc(node->buffer, node->buffer_capacity);
	}
	buffer[node->buffer_length] = c;
	node->buffer_length++;
}

static const char* __k_fs_procfs_state(process_t* proc) {
	switch(proc->state) {
		case PROCESS_STATE_STARTING:
			return "R (starting)";
		case PROCESS_STATE_RUNNING:
			return "R (running)";
		case PROCESS_STATE_SLEEPING:
			return "S (sleeping)";
		case PROCESS_STATE_FINISHED:
			return "Z (zombie)";
		default:
			return "Unknown";
	}
}

static void __k_fs_procfs_process_fill_status(fs_node_t* node) {
	process_t* process = k_proc_process_find_by_pid(node->inode);
	proc_fs_node_t* pnode = (proc_fs_node_t*) node;
	if(process) {
		pnode->buffer          = malloc(64);
		pnode->buffer_capacity = 64;
		fctprintf(__k_fs_procfs_write_buffer, pnode, 
		"Name:  %s\n"
		"State: %s\n"
		"Pid:   %d"
		, 
		process->name, __k_fs_procfs_state(process), process->pid);
	} else {
		k_warn("Tried to fetch status for non-existent process %d", node->inode);
	}
}

static const procdir_entry_t process_directory_entries[] = {
	{"status", __k_fs_procfs_process_fill_status}
};

#define PROCDIR_ENTRY_COUNT sizeof(process_directory_entries) / sizeof(procdir_entry_t)

static uint32_t __k_fs_procfs_process_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	proc_fs_node_t* pnode = (proc_fs_node_t*) node;
	
	if(pnode->buffer_length < offset) {
		return 0;
	}

	if(pnode->buffer_length < offset + size) {
		size = pnode->buffer_length - offset;
	}

	memcpy(buffer, pnode->buffer, size);

	return size;
}

static void __k_fs_procfs_process_close(fs_node_t * node) {
	proc_fs_node_t * entry = (proc_fs_node_t*) node;
	k_free(entry->buffer);
}

static void __k_fs_procfs_process_open(fs_node_t* node, unsigned char flags UNUSED) {
	proc_fs_node_t* pnode = (proc_fs_node_t*) node;
	pnode->entry->fill(node);
	node->size = pnode->buffer_length;
}

static proc_fs_node_t* __k_fs_procfs_create_node(const procdir_entry_t* entry, fs_node_t* parent) {
	proc_fs_node_t* node = k_calloc(sizeof(proc_fs_node_t), 1);
	strncpy(node->node.name, entry->name, sizeof(node->node.name));
	node->node.flags = VFS_FILE;
	node->node.inode = parent->inode;
	node->node.fs.read = &__k_fs_procfs_process_read;	
	node->node.fs.open = &__k_fs_procfs_process_open;	
	node->node.fs.close = &__k_fs_procfs_process_close;	
	node->entry = entry;
	return node;
}

static fs_node_t* __k_fs_procfs_process_finddir(fs_node_t* node, const char* path) {
	for(int i = 0; i < PROCDIR_ENTRY_COUNT; i++) {
		if(!strcmp(process_directory_entries[i].name, path)) {
			return (fs_node_t*) __k_fs_procfs_create_node(&process_directory_entries[i], node);
;
		}
	}
	return NULL;
}

static struct dirent* __k_fs_procfs_process_readdir(fs_node_t* node UNUSED, uint32_t index) {
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

	index -= 2;

	if(index < PROCDIR_ENTRY_COUNT) {
		struct dirent * out = malloc(sizeof(struct dirent));
		memset(out, 0x00, sizeof(struct dirent));
		out->ino = 0;
		strncpy(out->name, process_directory_entries[index].name, sizeof(out->name));
		return out;
	}

	return NULL;
}

static fs_node_t* __k_fs_procfs_directory_for_process(process_t* process) {
	char buff[256];
	snprintf(buff, 256, "%d", process->pid);

	fs_node_t* dir = k_fs_vfs_create_node(buff);
	dir->inode  = process->pid;
	dir->flags  = VFS_DIR;
	dir->fs.readdir = &__k_fs_procfs_process_readdir;
	dir->fs.finddir = &__k_fs_procfs_process_finddir;

	return dir;
}


static uint32_t __k_fs_procfs_self_readlink(fs_node_t* node, uint8_t* buf, uint32_t size) {
	return snprintf(buf, size, "/proc/%d", node->inode);
}

static fs_node_t* __k_fs_procfs_self_directory() {
	fs_node_t* node = k_fs_vfs_create_node("self");
	node->inode = k_proc_process_current()->pid;
	node->flags = VFS_SYMLINK;
	node->fs.readlink = &__k_fs_procfs_self_readlink;	
	return node;
}

static fs_node_t* __k_fs_procfs_root_finddir(fs_node_t* node UNUSED, const char* path) {
	if(!strlen(path)) {
		return NULL;
	}

	if(!strcmp(path, "self")) {
		return __k_fs_procfs_self_directory();
	}		

	if(isdigit(path[0])) {
		pid_t pid = atoi(path);
		process_t* proc = k_proc_process_find_by_pid(pid);
		if(!proc) {
			return NULL;
		}
		return __k_fs_procfs_directory_for_process(proc);
	}

	return NULL;
}

static struct dirent* __k_fs_procfs_root_readdir(fs_node_t* node UNUSED, uint32_t index) {
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
	fsroot->fs.readdir = &__k_fs_procfs_root_readdir;
	fsroot->fs.finddir = &__k_fs_procfs_root_finddir;
	return fsroot;
}

int k_fs_procfs_init() {
	return k_fs_vfs_mount_node("/proc", __k_fs_procfs_create_root());
}
