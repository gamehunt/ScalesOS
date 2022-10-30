#include <string.h>

char* strcat(char* destination, const char* source){
    char* ptr = destination + strlen(destination);
    while (*source != '\0') {
        *ptr++ = *source++;
    }
    *ptr = '\0';
    return destination;    
}