#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

FILE _stdin  = {.fd = 0};
FILE _stdout = {.fd = 1};
FILE _stderr = {.fd = 2};

FILE* stdin  = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

void _putchar(char c) {
	fwrite(&c, 1, 1, stdout);
}

int puts(const char *str) {
	uint32_t res = fwrite(str, 1, strlen(str), stdout);
	putchar('\n');
	return res;
}

int fprintf(FILE * stream, const char * format, ... ){
	va_list argptr;
    va_start(argptr, format);
	int result = vfprintf(stream, format, argptr);
	va_end(argptr);
	return result;
}

int fflush(FILE *stream){
  return 0;
}

FILE* fopen(const char *path, const char *mode){
  FILE* file = malloc(sizeof(FILE));
  file->fd = __sys_open((uint32_t) path, 0, 0);
  return file;
}

int fclose(FILE * stream){
  __sys_close(stream->fd);
  free(stream);
  return 0;
}

int fseek(FILE * stream, long offset, int whence){
  //TODO
}

long ftell(FILE * stream){
  //TODO
}

int   vfprintf(FILE * device, const char *format, va_list argptr){
	char* buffer = (char*) malloc(4096); //TODO implement it properly
	vsnprintf(buffer, 4096, format, argptr);
    int result = __sys_write(device->fd, strlen(buffer), (uint32_t) buffer);
	free(buffer);
	return result;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE * stream){
  return __sys_read(stream->fd, size * nmemb, (uint32_t) ptr);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream){
  return __sys_write(stream->fd, size * nmemb, (uint32_t) ptr);
}
