#ifndef __LD_TYPES_H
#define __LD_TYPES_H

#include <stdint.h>

typedef struct list{
	uint32_t   size;
	void**     data;
} list_t;

typedef struct hashmap{

} hashmap_t;

list_t* list_create();
void    list_free(list_t* list);
void    list_push_front(list_t* list, void* data);
void    list_push_back(list_t* list, void* data);
void*   list_pop_back(list_t* list);
void*   list_pop_front(list_t* list);
void*   list_head(list_t* list);
void*   list_tail(list_t* list);
void    list_delete_element(list_t* list, void* data);

#endif
