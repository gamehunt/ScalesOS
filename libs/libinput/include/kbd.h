#ifndef __LIB_INPUT_KBD_H
#define __LIB_INPUT_KBD_H

#include <stdint.h>

#define KBD_EVENT_FLAG_UP     (1 << 0)
#define KBD_EVENT_FLAG_EXT    (1 << 1)

#define KBD_MOD_ALT   (1 << 0)
#define KBD_MOD_SHIFT (1 << 1)
#define KBD_MOD_CTRL  (1 << 2)
#define KBD_MOD_CAPS  (1 << 3)

typedef struct {
	int     scancode;
	uint8_t flags;
} keyboard_packet_t;

keyboard_packet_t* input_kbd_create_packet(int scancode);
char               input_kbd_translate(int scancode, uint8_t mods);

#endif
