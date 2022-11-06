#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void  free(void* mem);

void  abort();

int   abs(int num);

int   atoi(const char* str);

#if !defined(__LIBK) && !defined(__KERNEL)
    char* getenv(const char* env_var);
    int   atexit(void(*func)(void));
    void  exit(int code);
#endif

#endif