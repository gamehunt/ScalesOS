#include "kbd.h"
#include <stdlib.h>

keyboard_packet_t* input_kbd_create_packet(int scancode) {
	int flags = 0;
	if(scancode & 0x80) {
		flags |= KBD_EVENT_FLAG_UP;
	}
	flags &= 0x7F;

	keyboard_packet_t* packet = calloc(1, sizeof(keyboard_packet_t));
	packet->scancode = scancode; 
	packet->flags    = flags;

	return packet;
}
