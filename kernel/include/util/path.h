#ifndef __K_UTIL_PATH_H
#define __K_UTIL_PATH_H

#include <stdint.h>

char*    k_util_path_canonize(const char* path);
uint32_t k_util_path_length  (const char* path);
char*    k_util_path_segment (const char* path, uint32_t seg);
char*    k_util_path_filename(const char* path);
char*    k_util_path_folder  (const char* path);
char*    k_util_path_join    (const char* base, const char* append);

#endif