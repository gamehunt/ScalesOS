#ifndef __UNISTD_H
#define __UNISTD_H

#include <sys/types.h>
#include <stddef.h>

#if !defined(__LIBK) && !defined(__KERNEL)

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

pid_t fork(void);
pid_t getpid(void);
       
uid_t getuid(void);
uid_t geteuid(void);

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);  
int execve(const char *filename, char *const argv [], char *const envp[]);  

unsigned int sleep(unsigned int seconds);
unsigned int usleep(unsigned long usec);

int dup(int oldfd);
int dup2(int oldfd, int newfd);

int chdir(const char *path);
int fchdir(int fd);  

char* getcwd(char buf[], size_t size);
char* get_current_dir_name(void);

void* mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
int   munmap(void *start, size_t length);
int   msync(void *start, size_t length, int flags);

size_t read(int fd, void *buf, size_t count);
size_t write(int fd, void *buf, size_t count);
int    open(const char *pathname, int flags);
int    close(int fd);

int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);

#endif

#endif
