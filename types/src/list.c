#include "list.h"

#include <stdlib.h>
#include <string.h>

#define EXTEND(mem, count, size) \
    count++; \
    if(count == 1){ \
        mem = malloc(size);\
    }else{\
        mem = realloc(mem, size * count);\
    }

list_t* list_create(){
	list_t* list = calloc(1, sizeof(list_t));
	return list;
}

void list_free(list_t* list){
	if(list->size){
		free(list->data);
	}
	free(list);
}

void list_push_front(list_t* list, void* data){
	EXTEND(list->data, list->size, sizeof(void*));
	if(list->size > 1) {
		memmove(list->data + 1, list->data, (list->size - 1) * sizeof(void*));
	}
	list->data[0] = data;
}

void list_push_back(list_t* list, void* data) {
	EXTEND(list->data, list->size, sizeof(void*));
	list->data[list->size - 1] = data;
}

void* list_pop_back(list_t* list){
	if(!list->size){
        return 0;
    }
    void* data = list->data[list->size - 1];
	list->size--;
	if(list->size) {
		list->data = realloc(list->data, list->size * sizeof(void*));
	} else {
		free(list->data);
		list->data = 0;
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
        free(list->data);
		list->data = 0;
    }else{
	    memmove(list->data, list->data + 1, list->size * sizeof(void*));
	    list->data = realloc(list->data, list->size * sizeof(void*));
    }
	return data;
}

void list_delete_element(list_t* list, void* data){
    for(uint32_t i = 0; i < list->size; i++){
        if(list->data[i] == data){
            list->data[i] = 0;
            list->size--;
            if(list->size){
				if(i != list->size) {
                	memmove(list->data + i, list->data + i + 1, (list->size - i) * sizeof(void*));
				}
                list->data = realloc(list->data, list->size * sizeof(void*));
            }else{
                free(list->data);
				list->data = 0;
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

uint8_t list_contains(list_t* list, void* data) {
	for(size_t i = 0; i < list->size; i++) {
		if(list->data[i] == data) {
			return 1;
		}
	}
	return 0;
}
