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
#define SYS_SIGACT       16
#define SYS_YIELD        17
#define SYS_INSMOD       18
#define SYS_READDIR      19
#define SYS_SEEK         20
#define SYS_MOUNT        21
#define SYS_UMOUNT       22
#define SYS_MKFIFO       23
#define SYS_DUP2         24
#define SYS_OPENPTY      25
#define SYS_IOCTL        26
#define SYS_UNAME        27
#define SYS_CHDIR        28
#define SYS_GETCWD       29
#define SYS_GETUID       30
#define SYS_GETEUID      31
#define SYS_MMAP         32
#define SYS_MUNMAP       33
#define SYS_MSYNC        34
#define SYS_STAT         35
#define SYS_LSTAT        36
#define SYS_FSTAT        37
#define SYS_MKDIR        38
#define SYS_SETHEAP      39
#define SYS_PRCTL        40

#define SYS_DEBUG        255

#if !defined(__LIBK) && !defined(__KERNEL)


uint32_t syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f);

#define UNUSED1  0
#define UNUSED2  0, 0
#define UNUSED3  0, 0, 0
#define UNUSED4  0, 0, 0, 0
#define UNUSED5  0, 0, 0, 0, 0

#define __sys_read(fd, size, buffer)  				  syscall(SYS_READ,  fd, buffer, size, UNUSED2)
#define __sys_write(fd, size, buffer) 				  syscall(SYS_WRITE, fd, buffer, size, UNUSED2)
#define __sys_open(path, flags, mode) 				  syscall(SYS_OPEN,  path, flags, mode, UNUSED2)
#define __sys_close(fd)               				  syscall(SYS_CLOSE, fd, UNUSED4)
#define __sys_fork()                  				  syscall(SYS_FORK, UNUSED5)
#define __sys_getpid()                				  syscall(SYS_GETPID, UNUSED5)
#define __sys_exit(code)              				  syscall(SYS_EXIT, code, UNUSED4)
#define __sys_grow(size)              				  syscall(SYS_GROW, size, UNUSED4)
#define __sys_waitpid(pid, status, options)     	  syscall(SYS_WAITPID, pid, status, options, UNUSED2)
#define __sys_exec(path, argv, envp)            	  syscall(SYS_EXEC, path, argv, envp, UNUSED2)
#define __sys_sleep(microseconds)               	  syscall(SYS_SLEEP, microseconds, UNUSED4)
#define __sys_reboot(op)                              syscall(SYS_REBOOT, op, UNUSED4)
#define __sys_times(tms)                        	  syscall(SYS_TIMES, tms, UNUSED4)
#define __sys_gettimeofday(tv, tz)              	  syscall(SYS_GETTIMEOFDAY, tv, tz, UNUSED3)
#define __sys_settimeofday(tv, tz)              	  syscall(SYS_SETTIMEOFDAY, tv, tz, UNUSED3)
#define __sys_kill(pid,sig)  					      syscall(SYS_KILL, pid, sig, UNUSED3)
#define __sys_sigact(sig, handler, old)               syscall(SYS_SIGACT, sig, handler, old, UNUSED2)
#define __sys_yield()                           	  syscall(SYS_YIELD, UNUSED5)
#define __sys_insmod(addr, size)                	  syscall(SYS_INSMOD, addr, size, UNUSED3)
#define __sys_readdir(dir, index, out)          	  syscall(SYS_READDIR, dir, index, out, UNUSED2)
#define __sys_seek(fd, offset, origin)          	  syscall(SYS_SEEK, fd, offset, origin, UNUSED2)
#define __sys_mount(path, device, type)         	  syscall(SYS_MOUNT, path, device, type, UNUSED2)
#define __sys_umount(path)                      	  syscall(SYS_UMOUNT, path, UNUSED4)
#define __sys_mkfifo(path, mode)                	  syscall(SYS_MKFIFO, path, mode, UNUSED3)
#define __sys_dup2(fd1, fd2)                    	  syscall(SYS_DUP2, fd1, fd2, UNUSED3)
#define __sys_openpty(master, slave, name, opts, ws)  syscall(SYS_OPENPTY, master, slave, opts, ws, UNUSED1)
#define __sys_ioctl(fd, req, args)                    syscall(SYS_IOCTL, fd, req, args, UNUSED2)
#define __sys_uname(ts)                               syscall(SYS_UNAME, ts, UNUSED4)
#define __sys_chdir(fd)                               syscall(SYS_CHDIR, fd, UNUSED4)
#define __sys_getcwd(buf, size)                       syscall(SYS_GETCWD, buf, size, UNUSED3)
#define __sys_getuid()                                syscall(SYS_GETUID, UNUSED5)
#define __sys_geteuid()                               syscall(SYS_GETEUID, UNUSED5)
#define __sys_mmap(start, length, prot, flags, file)  syscall(SYS_MMAP, start, length, prot, flags, file)
#define __sys_munmap(start, length)                   syscall(SYS_MUNMAP, start, length, UNUSED3)
#define __sys_msync(start, length, flags)             syscall(SYS_MSYNC, start, length, flags, UNUSED2)
#define __sys_stat(path, sb)                          syscall(SYS_STAT, path, sb, UNUSED3)
#define __sys_lstat(path, sb)                         syscall(SYS_LSTAT, path, sb, UNUSED3)
#define __sys_fstat(fd, sb)                           syscall(SYS_FSTAT, fd, sb, UNUSED3)
#define __sys_mkdir(path, mode)                       syscall(SYS_MKDIR, path, mode, UNUSED3)
#define __sys_setheap(addr)                           syscall(SYS_SETHEAP, addr, UNUSED4)
#define __sys_prctl(op, arg)                          syscall(SYS_PRCTL, op, arg, UNUSED3)

#define __sys_debug(type, a, b, c, d)                 syscall(SYS_DEBUG, type, a, b, c, d)

#endif

#endif
