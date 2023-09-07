#ifndef __K_DEV_VT_H
#define __K_DEV_VT_H

#include <stdint.h>

struct tty;

int  	k_dev_vt_init();
void 	k_dev_vt_handle_scancode(uint8_t v);
void    k_dev_vt_tty_callback(struct tty* tty);
int32_t k_dev_vt_change(uint8_t number, uint8_t clear);

#endif
