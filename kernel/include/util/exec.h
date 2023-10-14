#ifndef __K_UTIL_EXEC
#define __K_UTIL_EXEC

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int (*exec_func)(const char* path, int argc, const char* argv[], const char* envp[]);	
	size_t  length;
	uint8_t signature[4];
} executable_t;

void k_util_add_exec_format(executable_t exec);
int  k_util_exec(const char* path, int argc, const char* argv[], const char* envp[]);
int  k_util_exec_shebang(const char* path, int argc, const char* argv[], const char* envp[]);
int  k_util_exec_elf(const char* path, int argc,  const char* argv[], const char* envp[]);
#endif
