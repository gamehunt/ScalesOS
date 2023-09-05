#ifndef __UNISTD_H
#define __UNISTD_H

#include <sys/types.h>

#if !defined(__LIBK) && !defined(__KERNEL)

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

pid_t fork(void);
pid_t getpid(void);

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);  
int execve(const char *filename, char *const argv [], char *const envp[]);  

unsigned int sleep(unsigned int seconds);
unsigned int usleep(unsigned long usec);

int dup(int oldfd);
int dup2(int oldfd, int newfd);

#endif

#endif
