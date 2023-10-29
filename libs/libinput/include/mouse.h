#ifndef __LIB_INPUT_MOUSE_H
#define __LIB_INPUT_MOUSE_H

#include <kernel/dev/ps2.h>
#include <stdint.h>

typedef uint8_t buttons_t;

#define MOUSE_BUTTON_LEFT   (1 << 0)
#define MOUSE_BUTTON_RIGHT  (1 << 1)
#define MOUSE_BUTTON_MIDDLE (1 << 2)

#define MOUSE_RAW_DATA_LBTN (1 << 0)
#define MOUSE_RAW_DATA_RBTN (1 << 1)
#define MOUSE_RAW_DATA_MBTN (1 << 2)
#define MOUSE_RAW_DATA_SIGN (1 << 3)
#define MOUSE_RAW_DATA_XSGN (1 << 4)
#define MOUSE_RAW_DATA_YSGN (1 << 4)
#define MOUSE_RAW_DATA_XOVF (1 << 6)
#define MOUSE_RAW_DATA_YOVF (1 << 7)

typedef struct {
	int16_t   dx;
	int16_t   dy;
	buttons_t buttons;
} mouse_packet_t;

mouse_packet_t* input_mouse_create_packet(ps_mouse_packet_t* raw_packet);

#endif
