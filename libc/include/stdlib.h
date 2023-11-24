#ifndef __STDLIB_H
#define __STDLIB_H

#define __ATEXIT_MAX 32

#include <stddef.h>

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void  free(void* mem);
void  vfree(void* mem);
void* valloc(size_t size, size_t alignment);
void* realloc(void* old, size_t size);

void  abort();
int   abs(int num);
int   atoi(const char* str);
void* itoa(int input, char* buffer, int radix);

#if !defined(__LIBK) && !defined(__KERNEL)
	int   system(const char *str);
    char* getenv(const char* env_var);
	int   putenv(char* string);
	int   setenv(const char *name, const char *value, int overwrite);
	int   unsetenv(const char * str);
    int   atexit(void(*func)(void));
    void  exit(int code);
#endif

#endif
