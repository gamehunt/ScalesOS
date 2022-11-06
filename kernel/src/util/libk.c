#include "dev/fb.h"
#include "dev/serial.h"
#include "mem/heap.h"
#include "util/log.h"

#include <stddef.h>

void _putchar(char a){
    k_dev_serial_write(a);
    if(k_dev_fb_available()){
        uint32_t fg, bg;
        k_util_log_get_colors(&fg, &bg);
        k_dev_fb_putchar(a, fg, bg);
    }
}

void* _malloc(size_t size){
    return k_malloc(size);
}

void _free(void* mem){
    return k_free(mem);
}