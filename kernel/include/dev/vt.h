#ifndef __K_DEV_VT_H
#define __K_DEV_VT_H

#include <stdint.h>

int  k_dev_vt_init();
void k_dev_vt_handle_scancode(uint8_t v);

#endif
