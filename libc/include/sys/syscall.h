#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#include <stdint.h>

#define SYS_READ    	 0
#define SYS_WRITE   	 1
#define SYS_OPEN    	 2
#define SYS_CLOSE   	 3
#define SYS_FORK    	 4
#define SYS_GETPID  	 5
#define SYS_EXIT    	 6
#define SYS_GROW    	 7
#define SYS_WAITPID 	 8
#define SYS_EXEC    	 9
#define SYS_SLEEP   	 10
#define SYS_REBOOT  	 11
#define SYS_TIMES   	 12
#define SYS_GETTIMEOFDAY 13
#define SYS_SETTIMEOFDAY 14
#define SYS_KILL         15
#define SYS_SIGNAL       16
#define SYS_YIELD        17
#define SYS_INSMOD       18
#define SYS_READDIR      19
#define SYS_SEEK         20

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
#define __sys_reboot()                          __syscall(SYS_REBOOT, UNUSED5)
#define __sys_times(tms)                        __syscall(SYS_TIMES, tms, UNUSED4)
#define __sys_gettimeofday(tv, tz)              __syscall(SYS_GETTIMEOFDAY, tv, tz, UNUSED3)
#define __sys_settimeofday(tv, tz)              __syscall(SYS_SETTIMEOFDAY, tv, tz, UNUSED3)
#define __sys_kill(pid,sig)                     __syscall(SYS_KILL, pid, sig, UNUSED3)
#define __sys_signal(sig, handler)              __syscall(SYS_SIGNAL, sig, handler, UNUSED3)
#define __sys_yield()                           __syscall(SYS_YIELD, UNUSED5)
#define __sys_insmod(addr, size)                __syscall(SYS_INSMOD, addr, size, UNUSED3)
#define __sys_readdir(dir, index, out)          __syscall(SYS_READDIR, dir, index, out, UNUSED2)
#define __sys_seek(fd, offset, origin)          __syscall(SYS_SEEK, fd, offset, origin, UNUSED2)

#endif

#endif
