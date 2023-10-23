#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__LIBK) && !defined(__KERNEL)

#define BUFSIZE 512
#define EOF     -1

typedef struct {
    uint32_t fd;
	uint32_t buffer_size;
	char*    read_buffer;
	char*    write_buffer;
	uint32_t read_ptr;
	uint32_t read_buffer_offset;
	uint32_t write_ptr;
	uint32_t available;
	uint32_t real_offset;
	uint8_t  eof;
	char     ungetc;
} FILE;

typedef struct {
	uint32_t fd;
	uint32_t index;
} DIR;

extern FILE* stdin ;
extern FILE* stdout;
extern FILE* stderr;

FILE*  fopen(const char *path, const char *mode);
FILE*  fdopen(int fildes, const char *mode);
int    fclose(FILE * stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE* stream);
int    fseek(FILE * stream, long offset, int whence);
int    fprintf(FILE * stream, const char * format, ...);
int    fflush(FILE *stream);
long   ftell(FILE * stream);
int    vfprintf(FILE * device, const char *format, va_list ap);
int    puts(const char *str);
char   fgetc(FILE* s);
char*  fgets(char *s, int size, FILE *stream);
int    fputc(int ch, FILE *stream);
void   clearerr(FILE *stream);
int    feof(FILE *stream);
int    ferror(FILE *stream);
int    fileno(FILE *stream);  
void   setbuf(FILE *stream, char *buf);
void   rewind(FILE* stream);
int    remove(const char* path);
int    unlink(const char* path);
int    rmdir(const char* path);

#endif

#define O_RDONLY   (1 << 0)
#define O_WRONLY   (1 << 1)
#define O_RDWR     O_RDONLY | O_WRONLY

#define O_CREAT    (1 << 2)
#define O_TRUNC    (1 << 3)
#define O_APPEND   (1 << 4)

#define O_NOFOLLOW (1 << 5)
#define O_NOBLOCK  (1 << 6)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void putchar(char c);
int  printf(const char* format, ...);
int  sprintf(char* buffer, const char* format, ...);
int  snprintf(char* buffer, size_t count, const char* format, ...);
int  vsnprintf(char* buffer, size_t count, const char* format, va_list va);
int  vprintf(const char* format, va_list va);
int  fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif
