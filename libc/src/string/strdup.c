#include <stdlib.h>
#include <string.h>

char *strdup(const char *str){
    char* buff = malloc(strlen(str) + 1);
    strcpy(buff, str);
    return buff;
}