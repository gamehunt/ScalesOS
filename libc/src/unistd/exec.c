#include "sys/syscall.h"
#include <unistd.h>

int execl(const char *path, const char *arg, ...){

}

int execlp(const char *file, const char *arg, ...){

}

int execv(const char *path, char *const argv[]){

}

int execvp(const char *file, char *const argv[]){

}  

int execve(const char *filename, char *const argv [], char *const envp[]) {
	return __sys_exec((uint32_t) filename, (uint32_t) argv, (uint32_t) envp);
}
