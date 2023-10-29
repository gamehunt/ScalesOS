#ifndef __LIB_INPUT_KBD_H
#define __LIB_INPUT_KBD_H

#include <stdint.h>

#define KBD_EVENT_FLAG_UP (1 << 0)

typedef struct {
	int     scancode;
	uint8_t flags;
} keyboard_packet_t;

keyboard_packet_t* input_kbd_create_packet(int scancode);

#endif
