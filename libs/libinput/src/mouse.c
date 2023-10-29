#include "mouse.h"
#include <stdlib.h>

mouse_packet_t* input_mouse_create_packet(ps_mouse_packet_t* raw_packet) {
	if(raw_packet->data & MOUSE_RAW_DATA_XOVF || raw_packet->data & MOUSE_RAW_DATA_YOVF) {
		return NULL;
	}

	mouse_packet_t* packet = calloc(1, sizeof(mouse_packet_t));

	packet->dx = raw_packet->mx;
	packet->dy = raw_packet->my;

	if(raw_packet->data & MOUSE_RAW_DATA_XSGN) {
		packet->dx -= 0x100;
	}

	if(raw_packet->data & MOUSE_RAW_DATA_YSGN) {
		packet->dy -= 0x100;
	}

	if(raw_packet->data & MOUSE_RAW_DATA_LBTN) {
		packet->buttons |= MOUSE_BUTTON_LEFT;
	}

	if(raw_packet->data & MOUSE_RAW_DATA_RBTN) {
		packet->buttons |= MOUSE_BUTTON_RIGHT;
	}

	if(raw_packet->data & MOUSE_RAW_DATA_MBTN) {
		packet->buttons |= MOUSE_BUTTON_MIDDLE;
	}

	return packet;
}
