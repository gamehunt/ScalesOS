#ifndef __TYPES_LIST
#define __TYPES_LIST

#include <kernel/proc/spinlock.h>
#include <stdint.h>

typedef struct list{
	uint32_t   size;
	void**     data;
    spinlock_t lock;
} list_t;

list_t* list_create();
void    list_free(list_t* list);
void    list_push_front(list_t* list, void* data);
void    list_push_back(list_t* list, void* data);
void*   list_pop_back(list_t* list);
void*   list_pop_front(list_t* list);
void*   list_head(list_t* list);
void*   list_tail(list_t* list);
void    list_delete_element(list_t* list, void* data);
uint8_t list_contains(list_t* list, void* data);

#endif
