#ifndef __LIB_INPUT_MOUSE_H
#define __LIB_INPUT_MOUSE_H

#include <kernel/dev/ps2.h>
#include <stdint.h>

typedef uint8_t buttons_t;

#define MOUSE_BUTTON_LEFT   (1 << 0)
#define MOUSE_BUTTON_RIGHT  (1 << 1)
#define MOUSE_BUTTON_MIDDLE (1 << 2)

typedef struct {
	int8_t    dx;
	int8_t    dy;
	buttons_t buttons;
} mouse_packet_t;

mouse_packet_t* input_mouse_create_packet(ps_mouse_packet_t raw_packet);

#endif
