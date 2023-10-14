#include <int/syscall.h>
#include "dev/acpi.h"
#include "dev/timer.h"
#include "dev/tty.h"
#include "dirent.h"
#include "errno.h"
#include "fs/pipe.h"
#include "fs/vfs.h"
#include "kernel/mem/paging.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "mod/elf.h"
#include "mod/modules.h"
#include "sys/syscall.h"
#include <stdio.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"
#include "sys/tty.h"
#include "util/exec.h"
#include "util/log.h"
#include "shared.h"
#include "sys/times.h"
#include "sys/time.h"
#include "sys/signal.h"
#include "sys/utsname.h"
#include "sys/mman.h"
#include "util/panic.h"
#include "util/path.h"

#include <proc/process.h>
#include <scales/reboot.h>
#include <scales/mmap.h>
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
		if(ctx->edi == 0) {
			k_panic("Debug syscall.", ctx);
			__builtin_unreachable();
		} else {
			k_debug("Debug syscall.");
			k_debug("%.08x %.08x %.08x", ctx->eip, ctx->esp, ctx->ebp);
			return ctx;
		}
	}

	if(syscalls[ctx->eax]) {
		ctx->eax = syscalls[ctx->eax](ctx->ebx, ctx->ecx, ctx->edx, ctx->edi, ctx->esi);
	} else {
		k_warn("Unknown syscall: %d!", ctx->eax);
	}

	POST_INTERRUPT

    return ctx;
}

static uint32_t sys_read(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;
	if(!fds || fds->size <= fd || !fds->nodes[fd]) {
		return 0;
	}

	fd_t* fdt = fds->nodes[fd];

	// k_info("sys_read(): %d bytes at +%d in %s", count, fdt->offset, fdt->node->name);

	int32_t read = k_fs_vfs_read(fdt->node, fdt->offset, count, buffer);

	if(read > 0) {
		fdt->offset += read;
	}

	return read;
}

static uint32_t sys_write(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;
	if(!fds || fds->size <= fd || !fds->nodes[fd]) {
		return 0;
	}

	fd_t* fdt = fds->nodes[fd];

	int32_t written = k_fs_vfs_write(fdt->node, fdt->offset, count, buffer);

	if(written > 0) {
		fdt->offset += written;
	}
	
	return written;
}

static uint32_t sys_open(const char* path, uint16_t flags, uint8_t mode) {
	if(!IS_VALID_PTR((uint32_t) path) || !strlen(path)) {
		return -EINVAL;
	}

	process_t* cur = k_proc_process_current();

	char  fullpath[255];
	char* canonpath = NULL;
	
	if(path[0] != '/') {
		canonpath = k_util_path_canonize(cur->wd, path);
	} else {
		canonpath = k_util_path_canonize(path, "");
	}

	strncpy(fullpath, canonpath, sizeof(fullpath));
	k_free(canonpath);
	
	fs_node_t* node = k_fs_vfs_open(fullpath, flags);
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

static uint32_t sys_signal(int sig, signal_handler_t handler) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return 1;
	}

	process_t* proc = k_proc_process_current();
	
	signal_handler_t* old = proc->signals[sig].handler;

	proc->signals[sig].handler = handler;

	return (uint32_t) old;
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
	if(!fds || fds->size <= fd || !fds->nodes[fd]) {
		return 0;
	}
	fd_t* fdt = fds->nodes[fd];
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
	if(!fds || fds->size <= fd || !fds->nodes[fd]) {
		return 0;
	}
	fd_t* fdt = fds->nodes[fd];

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
	return k_proc_process_open_node(k_proc_process_current(), node);
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

	if(fd < 0 || (uint32_t) fd >= fds->size || !fds->nodes[fd]) {
		return -EINVAL;
	}

	fs_node_t* node = fds->nodes[fd]->node;

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

	if(fd < 0 || (uint32_t) fd >= fds->size || !fds->nodes[fd]) {
		return -EINVAL;
	}

	fs_node_t* node = fds->nodes[fd]->node;

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
	int   fd   = arg->fd;
	off_t offs = arg->offset; 

	uint8_t map_flags = PAGE_PRESENT | PAGE_USER;
	uint32_t pages = (length / 0x1000) + 1;

	process_t* proc = k_proc_process_current();

	if(prot & PROT_WRITE) {
		map_flags |= PAGE_WRITABLE;
	}

	if(flags == MAP_FIXED) {
		if((uint32_t) start % 0x1000) {
			return -EINVAL;
		}
		for(size_t i = 0; i < pages; i++) {
			if((((uint32_t) start + i * 0x1000) >= VIRTUAL_BASE) || k_mem_paging_virt2phys(((uint32_t) start) + i * 0x1000)) {
				return -EINVAL;
			}
		}			
		k_mem_paging_map_region((uint32_t) start, 0, pages, map_flags, 0);
		return (uint32_t) start;
	} else if (flags == MAP_PRIVATE) {
		uint32_t addr = proc->image.mmap_info.start;
		proc->image.mmap_info.start += pages * 0x1000;
		k_mem_paging_map_region((uint32_t) addr, 0, pages, map_flags, 0);
		return addr;
	} 

	return -EINVAL;
}

static uint32_t sys_munmap(void* start, size_t length) {
	uint32_t pages = (length / 0x1000) + 1;
	for(size_t i = 0; i < pages; i++) {
		if((((uint32_t) start + i * 0x1000) >= VIRTUAL_BASE) || !k_mem_paging_virt2phys(((uint32_t) start) + i * 0x1000)) {
			return -EINVAL;
		}
	}			
	k_mem_paging_unmap_region((uint32_t) start, pages);
	return 0;
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
DEFN_SYSCALL2(sys_signal, int, signal_handler_t);
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
	k_int_syscall_setup_handler(SYS_SIGNAL, REF_SYSCALL(sys_signal));
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
    
	return K_STATUS_OK;
}

void k_int_syscall_setup_handler(uint8_t syscall, syscall_handler_t handler) {
	syscalls[syscall] = handler;
}
