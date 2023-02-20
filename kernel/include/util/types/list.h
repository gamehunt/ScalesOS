#ifndef __K_UTIL_TYPES_LIST
#define __K_UTIL_TYPES_LIST

#include <stdint.h>

typedef struct list{
	uint32_t size;
	void**   data;
} list_t;

list_t* list_create();
void    list_free(list_t* list);
void    list_push_front(list_t* list, void* data);
void    list_push_back(list_t* list, void* data);
void*   list_pop_back(list_t* list);
void*   list_pop_front(list_t* list);

#endif
