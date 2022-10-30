#include "dev/serial.h"
#include "mem/heap.h"

#include <stddef.h>

void _putchar(char a){
    k_dev_serial_write(a);
}

void* _malloc(size_t size){
    return k_malloc(size);
}

void _free(void* mem){
    return k_free(mem);
}