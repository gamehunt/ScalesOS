#include <kernel/mod/modules.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>

K_STATUS load(){
    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("gptpart", &load, &unload)
