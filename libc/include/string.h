#ifndef __STRING_H
#define __STRING_H

#include <stddef.h>

void *memset (void *dest, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memcpy (void *dest, const void *src, size_t n);
int   memcmp(const void *buf1, const void *buf2, size_t count);

char*  strcpy(char* destination, const char* source);
char*  strtok(char* string, const char* delim);
size_t strlen(const char *s);
int    strcmp(const char* s1, const char* s2);
char*  strcat(char* destination, const char* source);
char*  strdup(const char *str);
char*  strchr(const char *str, int ch);
char*  strchrnul(const char *str, int ch);
size_t strcspn(const char * string1, const char * string2);
int    strncmp(const char * string1, const char * string2, size_t num);
long   strtol(const char *start, char **end, int radix);

#endif
