#include <int/syscall.h>
#include "fs/vfs.h"
#include "sys/syscall.h"
#include <stdio.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"
#include "util/log.h"
#include "shared.h"

#include <proc/process.h>
#include <string.h>

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
    process_t* cur = k_proc_process_current();
    memcpy((void*) &cur->syscall_state, ctx, sizeof(interrupt_context_t));
	if(syscalls[ctx->eax]) {
		ctx->eax = syscalls[ctx->eax](ctx->ebx, ctx->ecx, ctx->edx, ctx->edi, ctx->esi);
	} else {
		k_warn("Unknown syscall: %d!", ctx->eax);
	}
    return ctx;
}

static uint32_t sys_read(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;
	if(fds->size >= fd && !fds->nodes[fd]) {
		return 0;
	}
	return k_fs_vfs_read(fds->nodes[fd], 0, count, buffer);
}

static uint32_t sys_write(uint32_t fd, uint8_t* buffer, uint32_t count) {
	fd_list_t* fds = &k_proc_process_current()->fds;
	if(fds->size >= fd && !fds->nodes[fd]) {
		return 0;
	}
	return k_fs_vfs_write(fds->nodes[fd], 0, count, buffer);
}

static uint32_t sys_open(const char* path, uint16_t flags, uint8_t mode) {
	fs_node_t* node = k_fs_vfs_open(path);
	if(!node) {
		return 0;
	}
	return k_proc_process_open_node(k_proc_process_current(), node);
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
	uint32_t   addr    = current->image.heap + current->image.heap_size;
	k_proc_process_grow_heap(k_proc_process_current(), size);
	return addr;
}

DEFN_SYSCALL3(sys_read, uint32_t, uint8_t*, uint32_t);
DEFN_SYSCALL3(sys_write, uint32_t, uint8_t*, uint32_t);
DEFN_SYSCALL3(sys_open, const char*, uint16_t, uint8_t);
DEFN_SYSCALL1(sys_close, uint32_t);
DEFN_SYSCALL0(sys_fork);
DEFN_SYSCALL0(sys_getpid);
DEFN_SYSCALL1(sys_exit, uint32_t);
DEFN_SYSCALL1(sys_grow, int32_t);

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
	k_int_syscall_setup_handler(SYS_GROW, REF_SYSCALL(sys_grow));
    
	return K_STATUS_OK;
}

void k_int_syscall_setup_handler(uint8_t syscall, syscall_handler_t handler) {
	syscalls[syscall] = handler;
}
