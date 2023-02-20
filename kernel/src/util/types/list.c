#include "util/types/list.h"

#include "mem/heap.h"
#include "string.h"
#include <string.h>

list_t* list_create(){
	list_t* list = k_malloc(sizeof(list_t));
	list->size = 0;
	list->data = 0;
	return list;
}

void list_free(list_t* list){
	if(list->size){
		k_free(list->data);
	}
	k_free(list);
}

void list_push_front(list_t* list, void* data){
	EXTEND(list->data, list->size, sizeof(void*));
	memmove(list->data + 1, list->data, list->size - 1);
	list->data[0] = data;
}

void list_push_back(list_t* list, void* data){
	EXTEND(list->data, list->size, sizeof(void*));
	list->data[list->size - 1] = data;
}

void* list_pop_back(list_t* list){
	void* data = list->data[list->size - 1];
	list->size--;
	list->data = k_realloc(list->data, list->size);
	return data;
}

void* list_pop_front(list_t* list){
	void* data = list->data[0];
	list->size--;
	memmove(list->data, list->data + 1, list->size);
	list->data = k_realloc(list->data, list->size);
	return data;
}
