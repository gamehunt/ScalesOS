#include "dev/serial.h"

#include <stddef.h>

static char buffered[65535];
static uint16_t buff_idx = 0;

static void (*print_callback)(char* a, uint32_t size) = 0;

void _libk_set_print_callback(void (*pk)(char* a, uint32_t size)){
    if(pk && !print_callback){
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
    }else if(buff_idx < 65535){
        buffered[buff_idx] = a;
        buff_idx++;
    }
}
