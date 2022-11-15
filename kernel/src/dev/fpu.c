#include "kernel.h"
#include "util/log.h"
#include <dev/fpu.h>

extern uint8_t __k_dev_fpu_probe();
extern void    __k_dev_fpu_control_word(uint16_t w);


K_STATUS k_dev_fpu_init(){
    uint8_t has = __k_dev_fpu_probe();

    if(!has){
        k_warn("FPU not found");
        return K_STATUS_ERR_GENERIC;
    }

    k_info("Found FPU");

    __k_dev_fpu_control_word(0x37A); // Both division by zero and invalid operands cause exceptions.

    return K_STATUS_OK;
}