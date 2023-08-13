#include "dev/fb.h"
#include "dev/serial.h"
#include "fs/vfs.h"
#include "mem/heap.h"

#include <stddef.h>

static char buffered[65535];
static uint16_t buff_idx = 0;

static void (*print_callback)(char* a, uint32_t size) = 0;

void _libk_set_print_callback(void (*pk)(char* a, uint32_t size)){
    if(!print_callback){
        for(int i = 0; i < buff_idx; i++){
            pk(&buffered[i], 1);
        }
    }
    print_callback = pk;
}

void _putchar(char a){
    k_dev_serial_putchar(a);
    if(print_callback){
        print_callback(&a, 1);
    }else{
        buffered[buff_idx] = a;
        buff_idx++;
    }
}
