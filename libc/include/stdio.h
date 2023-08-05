#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__LIBK) && !defined(__KERNEL)

typedef struct{
    uint32_t fd;
} FILE;


extern FILE* stdin ;
extern FILE* stdout;
extern FILE* stderr;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE*  fopen(const char *path, const char *mode);
int    fclose(FILE * stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE * stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream);
int    fseek(FILE * stream, long offset, int whence);
int    fprintf(FILE * stream, const char * format, ...);
int    fflush(FILE *stream);
long   ftell(FILE * stream);
int    vfprintf(FILE * device, const char *format, va_list ap);
int    puts(const char *str);

#endif


void putchar(char c);
int printf(const char* format, ...);
int sprintf(char* buffer, const char* format, ...);
int snprintf(char* buffer, size_t count, const char* format, ...);
int vsnprintf(char* buffer, size_t count, const char* format, va_list va);
int vprintf(const char* format, va_list va);
int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
