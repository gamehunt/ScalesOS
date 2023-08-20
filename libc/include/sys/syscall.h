#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#include <stdint.h>

#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_FORK    4
#define SYS_GETPID  5
#define SYS_EXIT    6
#define SYS_GROW    7
#define SYS_WAITPID 8
#define SYS_EXEC    9
#define SYS_SLEEP   10

#if !defined(__LIBK) && !defined(__KERNEL)

extern uint32_t __syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f);

#define UNUSED1  0
#define UNUSED2  0, 0
#define UNUSED3  0, 0, 0
#define UNUSED4  0, 0, 0, 0
#define UNUSED5  0, 0, 0, 0, 0

#define __sys_read(fd, size, buffer)  			__syscall(SYS_READ,  fd, buffer, size, UNUSED2)
#define __sys_write(fd, size, buffer) 			__syscall(SYS_WRITE, fd, buffer, size, UNUSED2)
#define __sys_open(path, flags, mode) 			__syscall(SYS_OPEN,  path, flags, mode, UNUSED2)
#define __sys_close(fd)               			__syscall(SYS_CLOSE, fd, UNUSED4)
#define __sys_fork()                  			__syscall(SYS_FORK, UNUSED5)
#define __sys_getpid()                			__syscall(SYS_GETPID, UNUSED5)
#define __sys_exit(code)              			__syscall(SYS_EXIT, code, UNUSED4)
#define __sys_grow(size)              			__syscall(SYS_GROW, size, UNUSED4)
#define __sys_waitpid(pid, status, options)     __syscall(SYS_WAITPID, pid, status, options, UNUSED2)
#define __sys_exec(path, argv, envp)            __syscall(SYS_EXEC, path, argv, envp, UNUSED2)
#define __sys_sleep(microseconds)               __syscall(SYS_SLEEP, microseconds, UNUSED4)

#endif

#endif
