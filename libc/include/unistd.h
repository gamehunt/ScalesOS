#ifndef __UNISTD_H
#define __UNISTD_H

#include <sys/types.h>

#if !defined(__LIBK) && !defined(__KERNEL)

pid_t fork(void);
pid_t getpid(void);

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);  

#endif

#endif