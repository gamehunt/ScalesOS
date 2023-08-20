#include "util/types/list.h"

#include "mem/heap.h"
#include "string.h"
#include <string.h>

list_t* list_create(){
	list_t* list = k_calloc(1, sizeof(list_t));
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
	memmove(list->data + 1, list->data, (list->size - 1) * sizeof(void*));
	list->data[0] = data;
}

void list_push_back(list_t* list, void* data){
	EXTEND(list->data, list->size, sizeof(void*));
	list->data[list->size - 1] = data;
}

void* list_pop_back(list_t* list){
	if(!list->size){
        return 0;
    }
    void* data = list->data[list->size - 1];
	list->size--;
	if(list->size > 0) {
		list->data = k_realloc(list->data, list->size * sizeof(void*));
	} else {
		k_free(list->data);
	}
	return data;
}

void* list_pop_front(list_t* list){
    if(!list->size){
        return 0;
    }
	void* data = list->data[0];
	list->size--;
    if(!list->size){
        k_free(list->data);
    }else{
	    memmove(list->data, list->data + 1, list->size * sizeof(void*));
	    list->data = k_realloc(list->data, list->size * sizeof(void*));
    }
	return data;
}

void list_delete_element(list_t* list, void* data){
    for(uint32_t i = 0; i < list->size; i++){
        if(list->data[i] == data){
            list->data[i] = 0;
            list->size--;
            if(!list->size){
                memmove(list->data + i, list->data + i + 1, list->size * sizeof(void*));
                list->data = k_realloc(list->data, list->size * sizeof(void*));
            }else{
                k_free(list->data);
            }
            break;
        }
    }
}


void* list_head(list_t* list) {
	if(!list->size) {
		return 0;
	}
	return list->data[0];
}

void* list_tail(list_t* list) {
	if(!list->size) {
		return 0;
	}
	return list->data[list->size - 1];
}
