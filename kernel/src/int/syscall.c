#include <int/syscall.h>
#include "dev/acpi.h"
#include "dev/timer.h"
#include "dev/tty.h"
#include "dirent.h"
#include "errno.h"
#include "fs/pipe.h"
#include "fs/socket.h"
#include "fs/vfs.h"
#include "kernel/mem/memory.h"
#include "kernel/mem/paging.h"
#include "mem/heap.h"
#include "mem/mmap.h"
#include "mem/paging.h"
#include "mod/elf.h"
#include "mod/modules.h"
#include "sys/socket.h"
#include "sys/syscall.h"
#include <signal.h>
#include <stdio.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"
#include "sys/tty.h"
#include "util/exec.h"
#include "util/fd.h"
#include "util/log.h"
#include "shared.h"
#include "sys/times.h"
#include "sys/time.h"
#include "sys/select.h"
#include "sys/signal.h"
#include "sys/utsname.h"
#include "sys/mman.h"
#include "sys/stat.h"
#include "util/panic.h"
#include "util/path.h"
#include "util/types/list.h"

#include <proc/process.h>
#include <scales/reboot.h>
#include <scales/mmap.h>
#include <scales/prctl.h>
#include <string.h>
#include <stdio.h>

#define DEFN_SYSCALL0(fn) uint32_t __##fn(UNUSED uint32_t a,UNUSED uint32_t b,UNUSED uint32_t c,UNUSED uint32_t d,UNUSED uint32_t e) { return fn(); }
#define DEFN_SYSCALL1(fn, type) uint32_t __##fn(uint32_t a, UNUSED uint32_t b, UNUSED  uint32_t c, UNUSED  uint32_t d, UNUSED  uint32_t e) { return fn((type) a); }
#define DEFN_SYSCALL2(fn, type1, type2) uint32_t __##fn(uint32_t a, uint32_t b, UNUSED  uint32_t c, UNUSED  uint32_t d, UNUSED  uint32_t e) \
			{ return fn((type1) a, (type2) b); }
#define DEFN_SYSCALL3(fn, type1, type2, type3) uint32_t __##fn(uint32_t a, uint32_t b, uint32_t c, UNUSED  uint32_t d, UNUSED  uint32_t e) \
			{ return fn((type1) a, (type2) b, (type3) c); }
#define DEFN_SYSCALL4(fn, type1, type2, type3, type4) uint32_t __##fn(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UNUSED  uint32_t e) \
			{ return fn((type1) a, (type2) b, (type3) c, (type4) d); }
#define DEFN_SYSCALL5(fn, type1, type2, type3, type4, type5) uint32_t __##fn(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) \
			{ return fn((type1) a, (type2) b, (type3) c, (type4) d, (type5) e); }

#define REF_SYSCALL(fn) &__##fn

#define UNIMPLEMENTED k_warn("Syscall unimplemented: %s", __func__)


extern void _syscall_stub();

static syscall_handler_t syscalls[MAX_SYSCALL + 1];

interrupt_context_t* __k_int_syscall_dispatcher(interrupt_context_t* ctx){
	PRE_INTERRUPT

    process_t* cur = k_proc_process_current();
    memcpy((void*) &cur->syscall_state, ctx, sizeof(interrupt_context_t));

	if(ctx->eax == 255) {
		if(ctx->ebx == 0) {
			k_panic("Debug syscall.", ctx);
			__builtin_unreachable();
		} else {
			k_debug("Debug syscall.");
			k_debug("%.08x %.08x %.08x", ctx->eip, ctx->esp, ctx->ebp);
			k_debug("%.08x %.08x %.08x %.08x %.08x", ctx->ebx, ctx->ecx, ctx->edx, ctx->edi, ctx->esi);
			return ctx;
		}
	}

	long result = 0;

	if(syscalls[ctx->eax]) {
		result = syscalls[ctx->eax](ctx->ebx, ctx->ecx, ctx->edx, ctx->edi, ctx->esi);
	} else {
		k_warn("Unknown syscall: %d!", ctx->eax);
	}

	if(result == -ERESTARTSYS) {
		cur->interrupted_syscall = ctx->eax;
	}

	ctx->eax = result;

	POST_INTERRUPT

    return ctx;
}

static uint32_t sys_read(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return -ENOENT;
	}
	// k_info("sys_read(): %d bytes at +%d in %s", count, fdt->offset, fdt->node->name);

	int32_t read = k_fs_vfs_read(fdt->node, fdt->offset, count, buffer);

	if(read > 0) {
		fdt->offset += read;
	}

	return read;
}

static uint32_t sys_write(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return -ENOENT;
	}

	int32_t written = k_fs_vfs_write(fdt->node, fdt->offset, count, buffer);

	if(written > 0) {
		fdt->offset += written;
	}
	
	return written;
}

static char* __sys_to_fullpath(const char* path, process_t* process) {
	if(path[0] != '/') {
		return k_util_path_canonize(process->wd, path);
	} else {
		return k_util_path_canonize(path, "");
	}
}

static uint32_t sys_open(const char* path, uint16_t flags, uint8_t mode) {
	if(!IS_VALID_PTR((uint32_t) path) || !strlen(path)) {
		return -EINVAL;
	}

	process_t* cur = k_proc_process_current();

	char* fullpath = __sys_to_fullpath(path, cur);

	fs_node_t* node = k_fs_vfs_open(fullpath, flags);
	k_free(fullpath);

	if(!node) {
		return -ENOENT;
	}

	return k_proc_process_open_node(cur, node);
}

static uint32_t sys_close(uint32_t fd) {
	k_proc_process_close_fd(k_proc_process_current(), fd);
	return 0;
}

static uint32_t sys_fork() {
	return k_proc_process_fork();
}

static uint32_t sys_getpid() {
	return k_proc_process_current()->pid;
}

static uint32_t sys_exit(uint32_t code) {
	k_proc_process_exit(k_proc_process_current(), code);
	__builtin_unreachable();
}

static uint32_t sys_grow(int32_t size) {
	process_t* current = k_proc_process_current();
	if(!size) {
		return current->image.heap;
	}
	return (uint32_t) k_proc_process_grow_heap(current, size);
}

static uint32_t sys_exec(const char* path, char** argv, char** envp) {
	if(!IS_VALID_PTR((uint32_t) path)) {
		return -EINVAL;
	}

	int envc = 0;
	if(IS_VALID_PTR((uint32_t) envp)) {
		while(IS_VALID_PTR((uint32_t) *(envp + envc))) {
			envc++;	
		}
	}
	int argc = 0;
	if(IS_VALID_PTR((uint32_t) argv)) {
		while(IS_VALID_PTR((uint32_t) *(argv + argc))) {
			argc++;	
		}
	}
	char** _envp = !envc ? 0 : malloc((envc + 1) * sizeof(char*));
	for(int i = 0; i < envc; i++) {
		if(IS_VALID_PTR((uint32_t) envp[i])) {
			_envp[i] = strdup(envp[i]);	
		}
	}
	if(envc) {
		_envp[envc] = 0;
	}
	char** _argv = !argc ? 0 : malloc(argc * sizeof(char*));
	for(int i = 0; i < argc; i++) {
		if(IS_VALID_PTR((uint32_t) argv[i])) {
			_argv[i] = strdup(argv[i]);	
		}
	}
	return k_util_exec(path, argc, _argv, _envp); 
}

static uint32_t sys_waitpid(pid_t pid, int* status, int options) {
	return k_proc_process_waitpid(k_proc_process_current(), pid, status, options);
}

static uint32_t sys_sleep(uint64_t microseconds) {
	return k_proc_process_sleep(k_proc_process_current(), microseconds);
}

static uint32_t sys_reboot(int op) {
	if(op == SCALES_REBOOT_CMD_REBOOT) {
		k_dev_acpi_reboot();
		__builtin_unreachable();
	} else if(op == SCALES_REBOOT_CMD_SHUTDOWN) {
		k_dev_acpi_shutdown();
		__builtin_unreachable();
	}

	return -EINVAL;
}

static uint32_t sys_times(struct tms* tms) {
	if(tms && IS_VALID_PTR((uint32_t) tms)) {
		process_t* proc = k_proc_process_current();
		tms->tms_utime = proc->time_total;
		tms->tms_stime = proc->time_system;
	}
	return k_dev_timer_read_tsc() / k_dev_timer_get_core_speed();
}

static uint32_t sys_gettimeofday(struct timeval* tv, struct timezone* tz UNUSED) {
	if(!IS_VALID_PTR((uint32_t) tv)) {
		return 0;
	}

	uint64_t base    = k_dev_timer_tsc_base();
	uint64_t speed   = k_dev_timer_get_core_speed();
	uint64_t ticks   = k_dev_timer_read_tsc() / speed - base;
	uint64_t init_ts = k_dev_timer_get_initial_timestamp();

	tv->tv_sec  = ticks / 1000000 + init_ts;
	tv->tv_msec = ticks % 1000000;

	return 0;
}

static uint32_t sys_settimeofday(struct timeval* tv, struct timezone* tz) {
	UNIMPLEMENTED;
	return 0;
}

static uint32_t sys_sigact(int sig, const struct sigaction* handler, struct sigaction* prev) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return 1;
	}

	if(!IS_VALID_PTR((uint32_t) handler)) {
		return -EINVAL;
	}

	process_t* proc = k_proc_process_current();

	if(IS_VALID_PTR((uint32_t) prev)) {
		signal_handler_t old = proc->signals[sig].handler;
		prev->sa_handler = old;
	}

	if(handler->sa_handler == SIG_DFL) {
		proc->signals[sig].handler = 0;
		SET_SIGNAL_UNIGNORED(proc, sig);
	} else if(handler->sa_handler == SIG_IGN) {
		SET_SIGNAL_IGNORED(proc, sig);
	} else {
		SET_SIGNAL_UNIGNORED(proc, sig);
		proc->signals[sig].handler = handler->sa_handler;
		proc->signals[sig].flags   = handler->sa_flags;
	}

	return 0;
}

static uint32_t sys_kill(pid_t pid, int sig) {
	process_t* proc = k_proc_process_find_by_pid(pid);
	if(proc && proc->state != PROCESS_STATE_FINISHED) {
		k_proc_process_send_signal(proc, sig);
	}
	return 0;
}

static uint32_t sys_yield() {
	k_proc_process_yield();
	__builtin_unreachable();
}

static uint32_t sys_insmod(void* buffer, uint32_t size) {
	k_info("Loading module from 0x%x...", buffer);

	void* kernel_buffer = (void*) k_malloc(size);
	memcpy(kernel_buffer, buffer, size);

	module_info_t* mod = k_mod_elf_load_module(kernel_buffer);
	if(!mod || !mod->load) {
		k_info("-- Invalid header.");
		return 1;
	}
	k_info("Loading module %s...", mod->name);
	int result = mod->load();
	k_info("-- Finished with status %d", result);
	return result;
}

static uint32_t sys_readdir(uint32_t fd, uint32_t index, struct dirent* out) {
	fd_list_t* fds = &k_proc_process_current()->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return -ENOENT;
	}

	struct dirent* result = k_fs_vfs_readdir(fdt->node, index);

	if(!result) {
		return 0;
	}

	*out = *result;

	k_free(result);

	return 1;
}

static uint32_t sys_seek(uint32_t fd, uint32_t offset, uint8_t origin) {
	fd_list_t* fds = &k_proc_process_current()->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return -ENOENT;
	}

	switch(origin) {
		case SEEK_CUR:
			fdt->offset += offset;
			break;
		case SEEK_SET:
			fdt->offset = offset;
			break;
		case SEEK_END:
			fdt->offset = fdt->node->size + offset;
			break;
	}

	return fdt->offset;
}

uint32_t sys_mount(const char* path, const char* device, const char* type) {
	k_debug("Mounting: %s to %s as %s", device, path, type);
	return k_fs_vfs_mount(path, device, type);
}

uint32_t sys_umount(const char* path) {
	k_debug("Umounting: %s", path);
	return k_fs_vfs_umount(path);
}

uint32_t sys_mkfifo(const char* path, int mode UNUSED) {
	k_debug("Creating pipe at %s", path);
	fs_node_t* node = k_fs_pipe_create(4096);
	node->mode = O_RDWR;
	k_fs_vfs_mount_node(path, node);
	fs_node_t* copy = k_fs_vfs_dup(node);
	return k_proc_process_open_node(k_proc_process_current(), copy);
}

uint32_t sys_dup2(int fd1, int fd2) {
	process_t* proc = k_proc_process_current();

	fd_list_t* fds = &proc->fds;

	if(fd1 < 0) {
		return -EINVAL;
	}

	if((uint32_t) fd1 >= fds->size || !fds->nodes[fd1]) {
		return -EINVAL;
	}

	if(fd2 < 0) {
		fd2 = 0;
		while((uint32_t) fd2 < fds->size && fds->nodes[fd2]) {
			fd2++;
		}	
	}

	if((uint32_t) fd2 >= fds->size) {
		uint32_t difference = fds->size - fd2 + 1;
		fds->size += difference;
		fds->nodes = realloc(fds->nodes, fds->size * sizeof(fd_t*));
	}

	fds->nodes[fd2] = fds->nodes[fd1];
	fds->nodes[fd1]->links++;

	return fd2;
}

uint32_t sys_openpty(int* master, int* slave, char* name, struct termios* ts, struct winsize* ws) {
	if(!master || !slave) {
		return -EINVAL;
	}

	if(ts && !IS_VALID_PTR((uint32_t) ts)) {
		return -EINVAL;
	}

	if(ws && !IS_VALID_PTR((uint32_t) ws)) {
		return -EINVAL;
	}

	if(name && !IS_VALID_PTR((uint32_t) name)) {
		return -EINVAL;
	}

	fs_node_t* master_node;
	fs_node_t* slave_node;

	k_dev_tty_create_pty(ws, &master_node, &slave_node);

	master_node->mode = O_RDWR;
	slave_node->mode  = O_RDWR;

	process_t* process = k_proc_process_current();

	*master = k_proc_process_open_node(process, master_node);
	*slave  = k_proc_process_open_node(process, slave_node);

	return 0;
}

static uint32_t sys_ioctl(int fd, uint32_t req, void* args) {
	process_t* proc = k_proc_process_current();
	
	fd_list_t* fds = &proc->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return NULL;
	}

	fs_node_t* node = fdt->node;

	return k_fs_vfs_ioctl(node, req, args);
}

static uint32_t sys_uname(struct utsname* buf) {
	if(!IS_VALID_PTR((uint32_t) buf)) {
		return -EINVAL;
	}

	strncpy(buf->sysname,  "ScalesOS x86", sizeof(buf->sysname));
	strncpy(buf->version,  KERNEL_VERSION, sizeof(buf->version));
	strncpy(buf->release,  "1",            sizeof(buf->release));
	strncpy(buf->nodename, "\0",           sizeof(buf->nodename));
	strncpy(buf->machine,  "Unknown",      sizeof(buf->machine));

	return 0;
}

static uint32_t sys_getcwd(char buf[], size_t size) {
	process_t* cur = k_proc_process_current();

	const char* path = cur->wd;

	if(strlen(path) >= size) {
		return -ERANGE; 
	}

	strncpy(buf, path, size);

	return (uint32_t) buf;
}

static uint32_t sys_chdir(int fd) {
	process_t* cur = k_proc_process_current();

	fd_list_t* fds = &cur->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return NULL;
	}

	fs_node_t* node = fdt->node;

	if(!(node->flags & VFS_DIR)) {
		return -ENOTDIR;
	}

	k_fs_vfs_close(cur->wd_node);
	cur->wd_node = node;	

	strncpy(cur->wd, node->path, sizeof(cur->wd));

	return 0;
}

static uint32_t sys_getuid() {
	return k_proc_process_current()->uid;
}

static uint32_t sys_geteuid() {
	return sys_getuid(); 
}

static uint32_t sys_mmap(void* start, size_t length, int prot, int flags, file_arg_t* arg) {
	process_t* proc = k_proc_process_current();

	if(flags == MAP_ANONYMOUS) {
		mmap_block_t* block = k_mem_mmap_allocate_block(&proc->image.mmap_info, start, length, prot, flags);
		if(!block) {
			return -ENOMEM;
		}
		return block->start;
	}

	if(!IS_VALID_PTR((uint32_t) arg)) {
		return -EINVAL;
	}

	int   fd   = arg->fd;
	off_t offs = arg->offset; 

	fd_list_t* fds = &proc->fds;

	fd_t* fdt = fd2fdt(fds, fd);
	if(!fdt) {
		return -ENOENT;
	}

	if((flags & MAP_SHARED) && !(fdt->node->mode & O_RDWR)) {
		return -EACCES;
	}

	mmap_block_t* block = k_mem_mmap_allocate_block(&proc->image.mmap_info, start, length, prot, flags);
	if(!block) {
		return -ENOMEM;
	}

	block->offset = offs;
	block->fd     = fd;

	return block->start;
}

static uint32_t sys_msync(void* start, size_t length, int flags) {
	if((flags & MS_SYNC) && (flags & MS_ASYNC)) {
		return -EINVAL;
	}

	process_t* proc = k_proc_process_current();
	mmap_block_t* block = k_mem_mmap_get_mapping(&proc->image.mmap_info, (uint32_t) start);

	if(!block || block->end < (uint32_t)start + length) {
		return -EINVAL;
	}
	
	if(!(block->type & MAP_SHARED)) {
		return -EINVAL;
	}

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, block->fd);

	if(!fdt) {
		return -EFAULT;
	}

	k_mem_mmap_sync_block(block, 0);

	if(flags & MS_INVALIDATE) {
		list_t* mappings = k_mem_mmap_get_mappings_for_fd(block->fd, block->offset);
		for(size_t i = 0; i < mappings->size; i++) {
			mmap_block_t* shared_block = mappings->data[i];
			k_mem_mmap_mark_dirty(shared_block);
		}
		list_free(mappings);
	}
	
	return 0;
}

static uint32_t sys_munmap(void* start, size_t length) {
	process_t* proc = k_proc_process_current();
	mmap_block_t* block = k_mem_mmap_get_mapping(&proc->image.mmap_info, (uint32_t) start);

	if(!block || block->size < length) {
		return -EINVAL;
	}

	k_mem_mmap_sync_block(block, 0);

	k_mem_mmap_free_block(&proc->image.mmap_info, block);

	return 0;
}

static void __stat(fs_node_t* node, struct stat* sb) {
	sb->st_ino  = node->inode;
	sb->st_size = node->size;
	switch(node->flags) {
		case VFS_FILE:
			sb->st_mode = S_IFREG;
			break;
		case VFS_DIR:
			sb->st_mode = S_IFDIR;
			break;
		case VFS_SYMLINK:
			sb->st_mode = S_IFLNK;
			break;
		case VFS_CHARDEV:
			sb->st_mode = S_IFCHR;
			break;
		case VFS_FIFO:
			sb->st_mode = S_IFIFO;
			break;
	}
}

static uint32_t sys_stat(const char* path, struct stat* sb) {
	if(!IS_VALID_PTR((uint32_t) path)) {
		return -EINVAL;
	}

	if(!IS_VALID_PTR((uint32_t) sb)) {
		return -EINVAL;
	}

	process_t* cur = k_proc_process_current();

	char* fullpath = __sys_to_fullpath(path, cur);
	fs_node_t* node = k_fs_vfs_open(fullpath, 0);	

	if(!node) {
		return -ENOENT;
	}

	__stat(node, sb);

	k_fs_vfs_close(node);

	return 0;
}

static uint32_t sys_lstat(const char* path, struct stat* sb) {
	if(!IS_VALID_PTR((uint32_t) path)) {
		return -EINVAL;
	}

	if(!IS_VALID_PTR((uint32_t) sb)) {
		return -EINVAL;
	}

	process_t* cur = k_proc_process_current();
	
	char* fullpath = __sys_to_fullpath(path, cur);
	fs_node_t* node = k_fs_vfs_open(fullpath, 0);	

	if(!node) {
		return -ENOENT;
	}

	__stat(node, sb);

	k_fs_vfs_close(node);

	return 0;
}

static uint32_t sys_fstat(int fd, struct stat* sb) {
	if(!IS_VALID_PTR((uint32_t) sb)) {
		return -EINVAL;
	}

	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, fd);

	if(!fdt) {
		return -ENOENT;
	}

	fs_node_t* node = fdt->node;

	__stat(node, sb);

	return 0;
}

uint32_t sys_mkdir(const char* path, mode_t mode) {
	process_t* cur = k_proc_process_current();

	char* fullpath = __sys_to_fullpath(path, cur);

	char* name   = k_util_path_filename(fullpath);
	char* parent = k_util_path_folder(fullpath);

	fs_node_t* par_node = k_fs_vfs_open(parent, mode);

	k_free(parent);
	k_free(fullpath);

	if(!par_node) {
		return -ENOENT;
	}

	k_fs_vfs_mkdir(par_node, name, mode);

	k_free(name);

	k_fs_vfs_close(par_node);

	return 0;
}

uint32_t sys_setheap(void* addr) {
	if((uint32_t) addr >= USER_STACK_END) {
		return -EINVAL;
	}

	process_t* proc = k_proc_process_current();

	proc->image.heap      = (uint32_t) addr;
	proc->image.heap_size = USER_HEAP_INITIAL_SIZE;

	return 0;
}

uint32_t sys_prctl(int op, void* arg) {
	process_t* process = k_proc_process_current();

	switch(op) {
		case PRCTL_SETNAME:
			if(!IS_VALID_PTR((uint32_t) arg)) {
				return -EINVAL;
			}
			strncpy(process->name, arg, sizeof(process->name));
			return 0;
		default:
			return -EINVAL;
	}
}

uint32_t sys_rm(const char* path) {
	process_t* cur = k_proc_process_current();

	char* fullpath = __sys_to_fullpath(path, cur);
	fs_node_t* node = k_fs_vfs_open(fullpath, 0);	

	k_free(fullpath);

	if(!node) {
		return -ENOENT;
	}

	return k_fs_vfs_rm(node);
}

uint32_t sys_rmdir(const char* path) {
	process_t* cur = k_proc_process_current();

	char* fullpath  = __sys_to_fullpath(path, cur);
	fs_node_t* node = k_fs_vfs_open(fullpath, 0);	

	k_free(fullpath);

	if(!node) {
		return -ENOENT;
	}

	return k_fs_vfs_rmdir(node);
}

static uint32_t sys_socket(int d, int t, int p) {
	fs_node_t* node = k_fs_socket_create(d, t, p);

	if(!node) {
		return -EINVAL;
	}

	return k_proc_process_open_node(k_proc_process_current(), node);
}

static uint32_t sys_bind(int fd, struct sockaddr* addr, socklen_t l) {
	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, fd);

	if(!fdt) {
		return -ENOENT;
	}

	fs_node_t* node = fdt->node;

	return k_fs_socket_bind(node->device, addr, l);
}

static uint32_t sys_connect(int fd, struct sockaddr* addr, socklen_t l) {
	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, fd);

	if(!fdt) {
		return -ENOENT;
	}

	fs_node_t* node = fdt->node;

	return k_fs_socket_connect(node->device, addr, l);
}

static uint32_t sys_listen(int fd, int backlog UNUSED) {
	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, fd);

	if(!fdt) {
		return -ENOENT;
	}

	fs_node_t* node = fdt->node;

	return k_fs_socket_listen(node->device);
}

static uint32_t sys_accept(int fd, struct sockaddr* addr, socklen_t* l) {
	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	fd_t* fdt = fd2fdt(list, fd);

	if(!fdt) {
		return -ENOENT;
	}

	fs_node_t* node = fdt->node;

	int32_t r = (int32_t) k_fs_socket_accept(node->device);
	if(r < 0) {
		return r;
	}

	fs_node_t* new = (fs_node_t*) r;
	socket_t* sock = new->device;

	if(addr) {
		*l = sock->addr_len;
		memcpy(addr, sock->addr, sock->addr_len);
	}

	return k_proc_process_open_node(proc, new);
}

static int __select_for_event(int fd, fs_node_t* node, fd_set* set, fd_set* wait_set, uint8_t event) {
	if(!set) {
		return 0;
	}
	if(FD_ISSET(fd, set)) {
		FD_CLR(fd, set);
		if(node) {
			int r = k_fs_vfs_check(node, event);
			if(r < 0) {
				return -1;
			} else if (r == 0) {
				FD_SET(fd, wait_set);
				return 0;
			} else {
				FD_SET(fd, set);
				return 1;
			}
		} else {
			return -1;
		}
	}
	return 0;
}

static uint32_t sys_select(int n, fd_set* rs, fd_set* ws, fd_set* es, struct timeval* tv) {
	if(n < 0 || (tv && !IS_VALID_PTR((uint32_t) tv))) {
		return -EINVAL;
	}

	process_t* cur = k_proc_process_current();

	fd_list_t* fds = &cur->fds;

	int has_result = 0;

	fd_set wait_read;
	fd_set wait_write;
	fd_set wait_exc;

	FD_ZERO(&wait_read);
	FD_ZERO(&wait_write);
	FD_ZERO(&wait_exc);

	for(int i = 0; i < n; i++) {
		fd_t* fdt = fd2fdt(fds, i);
		fs_node_t* node = fdt ? fdt->node : NULL;

		int r = __select_for_event(i, node, rs, &wait_read, VFS_EVENT_READ);
		if(r > 0) {
			has_result++;
		} else if (r < 0) {
			return -EBADF;
		}

		r = __select_for_event(i, node, ws, &wait_write, VFS_EVENT_WRITE);
		if(r > 0) {
			has_result++;
		} else if (r < 0) {
			return -EBADF;
		}

		r = __select_for_event(i, node, es, &wait_exc, VFS_EVENT_EXCEPT);
		if(r > 0) {
			has_result++;
		} else if (r < 0) {
			return -EBADF;
		}
	}	

	if(has_result) {
		return has_result;
	}

	for(int i = 0; i < n; i++) {
		fd_t* fdt = fd2fdt(fds, i);
		if(!fdt) {
			continue;
		}
		
		fs_node_t* node = fdt->node;
		if(!node) {
			continue;
		}

		if(FD_ISSET(i, &wait_read)) {
			if(k_fs_vfs_wait(node, VFS_EVENT_READ, cur) < 0) {
				k_warn("wait(): failed for %s", node->name);		
			}
		}
		if(FD_ISSET(i, &wait_write)) {
			if(k_fs_vfs_wait(node, VFS_EVENT_WRITE, cur) < 0) {
				k_warn("wait(): failed for %s", node->name);		
			}
		}
		if(FD_ISSET(i, &wait_exc)) {
			if(k_fs_vfs_wait(node, VFS_EVENT_EXCEPT, cur) < 0) {
				k_warn("wait(): failed for %s", node->name);		
			}
		}
	}

	if(tv) {
		k_proc_process_timeout(cur, tv->tv_sec, tv->tv_msec);
	}

	cur->flags &= ~PROCESS_FLAG_INTERRUPTED;
	cur->state  = PROCESS_STATE_SLEEPING;
	k_proc_process_yield();

	if(cur->flags & PROCESS_FLAG_INTERRUPTED) {
		return -EINTR;
	}

	if(!cur->select_wait_node) {
		cur->select_wait_node  = 0;
		cur->select_wait_event = 0;
		return 0;
	}

	fd_set* sel_set = NULL;

	switch(cur->select_wait_event) {
		case VFS_EVENT_READ:
			sel_set = rs;
			break;
		case VFS_EVENT_WRITE:
			sel_set = ws;
			break;
		case VFS_EVENT_EXCEPT:
			sel_set = es;
			break;
	}

	if(sel_set) {
		for(int i = 0; i < n; i++) {
			fd_t* fd = fd2fdt(fds, i);
			if(!fd) {
				continue;
			}
			fs_node_t* node = fd->node;
			if(node == cur->select_wait_node) {
				FD_SET(i, sel_set);
				has_result = 1;
				break;
			}
		}
	}

	cur->select_wait_event = 0;
	cur->select_wait_node  = NULL;

	if(sel_set) {
		return has_result;
	} else {
		return -EFAULT;
	}
} 

DEFN_SYSCALL3(sys_read, uint32_t, uint8_t*, uint32_t);
DEFN_SYSCALL3(sys_write, uint32_t, uint8_t*, uint32_t);
DEFN_SYSCALL3(sys_open, const char*, uint16_t, uint8_t);
DEFN_SYSCALL1(sys_close, uint32_t);
DEFN_SYSCALL0(sys_fork);
DEFN_SYSCALL0(sys_getpid);
DEFN_SYSCALL1(sys_exit, uint32_t);
DEFN_SYSCALL3(sys_exec, const char*, char**, char**);
DEFN_SYSCALL1(sys_grow, int32_t);
DEFN_SYSCALL3(sys_waitpid, pid_t, int*, int);
DEFN_SYSCALL1(sys_sleep, uint64_t);
DEFN_SYSCALL1(sys_reboot, int);
DEFN_SYSCALL1(sys_times, struct tms*);
DEFN_SYSCALL2(sys_gettimeofday, struct timeval*, struct timezone*);
DEFN_SYSCALL2(sys_settimeofday, struct timeval*, struct timezone*);
DEFN_SYSCALL3(sys_sigact, int, const struct sigaction*, struct sigaction*);
DEFN_SYSCALL2(sys_kill, pid_t, int);
DEFN_SYSCALL0(sys_yield);
DEFN_SYSCALL2(sys_insmod, void*, uint32_t);
DEFN_SYSCALL3(sys_readdir, uint32_t, uint32_t, struct dirent*);
DEFN_SYSCALL3(sys_seek, uint32_t, uint32_t, uint8_t);
DEFN_SYSCALL3(sys_mount, const char*, const char*, const char*);
DEFN_SYSCALL1(sys_umount, const char*);
DEFN_SYSCALL2(sys_mkfifo, const char*, int);
DEFN_SYSCALL2(sys_dup2, int, int);
DEFN_SYSCALL5(sys_openpty, int*, int*, char*, struct termios*, struct winsize*);
DEFN_SYSCALL3(sys_ioctl, int, uint32_t, void*);
DEFN_SYSCALL1(sys_uname, struct utsname*);
DEFN_SYSCALL2(sys_getcwd, char*, size_t);
DEFN_SYSCALL1(sys_chdir, int);
DEFN_SYSCALL0(sys_getuid);
DEFN_SYSCALL0(sys_geteuid);
DEFN_SYSCALL5(sys_mmap, void*, size_t, int, int, file_arg_t*);
DEFN_SYSCALL2(sys_munmap, void*, size_t);
DEFN_SYSCALL3(sys_msync, void*, size_t, int);
DEFN_SYSCALL2(sys_stat,  const char*, struct stat*);
DEFN_SYSCALL2(sys_lstat, const char*, struct stat*);
DEFN_SYSCALL2(sys_fstat, int, struct stat*);
DEFN_SYSCALL2(sys_mkdir, const char*, mode_t);
DEFN_SYSCALL1(sys_setheap, void*);
DEFN_SYSCALL2(sys_prctl, int, void*);
DEFN_SYSCALL1(sys_rm, const char*);
DEFN_SYSCALL1(sys_rmdir, const char*);
DEFN_SYSCALL3(sys_socket, int, int, int);
DEFN_SYSCALL3(sys_bind, int, struct sockaddr*, socklen_t);
DEFN_SYSCALL3(sys_connect, int, struct sockaddr*, socklen_t);
DEFN_SYSCALL3(sys_accept, int, struct sockaddr*, socklen_t*);
DEFN_SYSCALL2(sys_listen, int, int);
DEFN_SYSCALL5(sys_select, int, fd_set*, fd_set*, fd_set*, struct timeval*);

K_STATUS k_int_syscall_init(){
	memset(syscalls, 0, sizeof(syscall_handler_t) * 256);
    k_int_idt_create_entry(0x80, (uint32_t) &_syscall_stub, 0x8, 0x3, 0xE);

	k_int_syscall_setup_handler(SYS_READ,  REF_SYSCALL(sys_read));
	k_int_syscall_setup_handler(SYS_WRITE, REF_SYSCALL(sys_write));
	k_int_syscall_setup_handler(SYS_OPEN,  REF_SYSCALL(sys_open));
	k_int_syscall_setup_handler(SYS_CLOSE, REF_SYSCALL(sys_close));
	k_int_syscall_setup_handler(SYS_FORK,  REF_SYSCALL(sys_fork));
	k_int_syscall_setup_handler(SYS_GETPID,  REF_SYSCALL(sys_getpid));
	k_int_syscall_setup_handler(SYS_EXIT, REF_SYSCALL(sys_exit));
	k_int_syscall_setup_handler(SYS_EXEC, REF_SYSCALL(sys_exec));
	k_int_syscall_setup_handler(SYS_GROW, REF_SYSCALL(sys_grow));
	k_int_syscall_setup_handler(SYS_WAITPID, REF_SYSCALL(sys_waitpid));
	k_int_syscall_setup_handler(SYS_SLEEP, REF_SYSCALL(sys_sleep));
	k_int_syscall_setup_handler(SYS_REBOOT, REF_SYSCALL(sys_reboot));
	k_int_syscall_setup_handler(SYS_TIMES, REF_SYSCALL(sys_times));
	k_int_syscall_setup_handler(SYS_GETTIMEOFDAY, REF_SYSCALL(sys_gettimeofday));
	k_int_syscall_setup_handler(SYS_SETTIMEOFDAY, REF_SYSCALL(sys_settimeofday));
	k_int_syscall_setup_handler(SYS_SIGACT, REF_SYSCALL(sys_sigact));
	k_int_syscall_setup_handler(SYS_KILL, REF_SYSCALL(sys_kill));
	k_int_syscall_setup_handler(SYS_YIELD, REF_SYSCALL(sys_yield));
	k_int_syscall_setup_handler(SYS_INSMOD, REF_SYSCALL(sys_insmod));
	k_int_syscall_setup_handler(SYS_READDIR, REF_SYSCALL(sys_readdir));
	k_int_syscall_setup_handler(SYS_SEEK, REF_SYSCALL(sys_seek));
	k_int_syscall_setup_handler(SYS_MOUNT, REF_SYSCALL(sys_mount));
	k_int_syscall_setup_handler(SYS_UMOUNT, REF_SYSCALL(sys_umount));
	k_int_syscall_setup_handler(SYS_MKFIFO, REF_SYSCALL(sys_mkfifo));
	k_int_syscall_setup_handler(SYS_DUP2, REF_SYSCALL(sys_dup2));
	k_int_syscall_setup_handler(SYS_OPENPTY, REF_SYSCALL(sys_openpty));
	k_int_syscall_setup_handler(SYS_IOCTL, REF_SYSCALL(sys_ioctl));
	k_int_syscall_setup_handler(SYS_UNAME, REF_SYSCALL(sys_uname));
	k_int_syscall_setup_handler(SYS_CHDIR, REF_SYSCALL(sys_chdir));
	k_int_syscall_setup_handler(SYS_GETCWD, REF_SYSCALL(sys_getcwd));
	k_int_syscall_setup_handler(SYS_GETUID, REF_SYSCALL(sys_getuid));
	k_int_syscall_setup_handler(SYS_GETEUID, REF_SYSCALL(sys_geteuid));
	k_int_syscall_setup_handler(SYS_MMAP, REF_SYSCALL(sys_mmap));
	k_int_syscall_setup_handler(SYS_MUNMAP, REF_SYSCALL(sys_munmap));
	k_int_syscall_setup_handler(SYS_MSYNC, REF_SYSCALL(sys_msync));
	k_int_syscall_setup_handler(SYS_STAT, REF_SYSCALL(sys_stat));
	k_int_syscall_setup_handler(SYS_LSTAT, REF_SYSCALL(sys_lstat));
	k_int_syscall_setup_handler(SYS_FSTAT, REF_SYSCALL(sys_fstat));
	k_int_syscall_setup_handler(SYS_MKDIR, REF_SYSCALL(sys_mkdir));
	k_int_syscall_setup_handler(SYS_SETHEAP, REF_SYSCALL(sys_setheap));
	k_int_syscall_setup_handler(SYS_PRCTL, REF_SYSCALL(sys_prctl));
	k_int_syscall_setup_handler(SYS_RM, REF_SYSCALL(sys_rm));
	k_int_syscall_setup_handler(SYS_RMDIR, REF_SYSCALL(sys_rmdir));
	k_int_syscall_setup_handler(SYS_SOCKET, REF_SYSCALL(sys_socket));
	k_int_syscall_setup_handler(SYS_BIND, REF_SYSCALL(sys_bind));
	k_int_syscall_setup_handler(SYS_ACCEPT, REF_SYSCALL(sys_accept));
	k_int_syscall_setup_handler(SYS_LISTEN, REF_SYSCALL(sys_listen));
	k_int_syscall_setup_handler(SYS_CONNECT, REF_SYSCALL(sys_connect));
	k_int_syscall_setup_handler(SYS_SELECT, REF_SYSCALL(sys_select));
    
	return K_STATUS_OK;
}

void k_int_syscall_setup_handler(uint8_t syscall, syscall_handler_t handler) {
	syscalls[syscall] = handler;
}
