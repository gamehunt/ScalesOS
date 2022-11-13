#include <stdio.h>
#include <util/path.h>
#include <string.h>
#include "mem/heap.h"
#include "util/log.h"

char* k_util_path_canonize(const char* p){
    char* path = strdup(p);
    if(!strcmp(path, "/")){
        return path;
    }
    uint32_t len = strlen(path);
    if(path[0] == '/'){
        memmove(path, path + 1, len);
        len--;
    }
    if(path[len - 1] == '/'){
        path[len - 1] = '\0';
    }
    return path;
}

uint32_t k_util_path_length(const char* path){
    if(!strcmp(path, "/")){
        return 0;
    }
    char* s = k_util_path_canonize(path);
    uint32_t l = 0;
    for(uint32_t i = 0; i < strlen(s); i++){
        if(s[i] == '/'){
            l++;
        }
    }
    k_free(s);
    return l + 1;
}

char* k_util_path_segment (const char* path, uint32_t seg){
    char* s = k_util_path_canonize(path);
    uint32_t iter = 0;
    uint32_t len = strlen(s);
    uint32_t sc = 0;
    while(sc < seg){
        uint32_t l = strcspn(&s[iter], "/");
        sc++;
        if(iter + l + 1 >= len){
            break;
        }
        iter += l + 1;
    }
    if(sc != seg){
        k_free(s);
        return 0;
    }
    *strchrnul(&s[iter], '/') = '\0';
    uint32_t new_len = strlen(&s[iter]);
    memmove(s, &s[iter], new_len + 1);
    return s;
}

char* k_util_path_filename(const char* path){
    uint32_t length = k_util_path_length(path);
    if(!length){
        return 0;
    }
    return k_util_path_segment(path, length - 1);
}

char* k_util_path_folder(const char* path){
    char* file = k_util_path_filename(path);
    char* p    = k_util_path_canonize(path);
    k_free(file);
    return p;
}

char* k_util_path_join(const char* base, const char* append){
    char* s1 = k_util_path_canonize(base);
    char* s2 = k_util_path_canonize(append);
    char* buffer = k_malloc(strlen(base) + strlen(append) + 2);
    sprintf(buffer, "%s/%s", s1, s2);
    k_free(s1);
    k_free(s2);
    return buffer;
}