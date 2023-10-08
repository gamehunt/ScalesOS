#include <stdio.h>
#include <util/path.h>
#include <string.h>
#include "mem/heap.h"
#include "util/log.h"
#include "util/types/list.h"

// /abc/../abc/file.txt -> /abc/file.txt
char* k_util_path_canonize(const char* root, const char* relative){
    if(!strcmp(relative, "/")){
        return strdup(relative);
    }

	list_t* stack = list_create();
	size_t  rel_size = strlen(relative);

	if(!rel_size || relative[0] != '/') {
		for(uint32_t i = 0; i < k_util_path_length(root); i++) {
			char* seg = k_util_path_segment(root, i);
			
			if(!strcmp(seg, ".")) {
				k_free(seg);
				continue;
			}

			if(!strcmp(seg, "..")) {
				if(stack->size > 0) {
					list_pop_back(stack);
				}
				k_free(seg);
				continue;
			}

			list_push_back(stack, seg);	
		}
	}

	if(rel_size) {
		for(uint32_t i = 0; i < k_util_path_length(relative); i++) {
			char* seg = k_util_path_segment(relative, i);

			if(!strcmp(seg, ".")) {
				k_free(seg);
				continue;
			}

			if(!strcmp(seg, "..")) {
				if(stack->size > 0) {
					list_pop_back(stack);
				}
				k_free(seg);
				continue;
			}

			list_push_back(stack, seg);
		}
	}

	char* path = malloc(255);
	strcpy(path, "/");

	for(uint32_t i = 0; i < stack->size; i++) {
		strncat(path, (char*) stack->data[i], 255);
		k_free(stack->data[i]);
		if(i != stack->size - 1) {
			strncat(path, "/", 255);
		}
	}

	list_free(stack);

	return path;
}

// /abc/ade/ -> abc/ade 
char* k_util_path_strip(const char* p){
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
    char* s = k_util_path_strip(path);
    uint32_t l = 0;
    for(uint32_t i = 0; i < strlen(s); i++){
        if(s[i] == '/'){
            l++;
        }
    }
    k_free(s);
    return l + 1;
}

char* k_util_path_segment(const char* path, uint32_t seg){
    char* s = k_util_path_strip(path);
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
	if(!strcmp(path, "/")) {
		return strdup(path);
	}

    uint32_t length = k_util_path_length(path);
    if(!length){
        return 0;
    }
    return k_util_path_segment(path, length - 1);
}

char* k_util_path_folder(const char* path){
    char* file = k_util_path_filename(path);
    char* p    = k_util_path_strip(path);
    k_free(file);
    return p;
}

char* k_util_path_join(const char* base, const char* append){
    char* s1 = k_util_path_strip(base);
    char* s2 = k_util_path_strip(append);
    char* buffer = k_malloc(strlen(base) + strlen(append) + 2);
    sprintf(buffer, "%s/%s", s1, s2);
    k_free(s1);
    k_free(s2);
    return buffer;
}
