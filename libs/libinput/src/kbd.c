#include "kbd.h"
#include "input/keys.h"
#include <ctype.h>
#include <stdlib.h>

static const char __scancodes[128] = { 
	0,
	27, // ESC
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=',
	'\b',
	'\t',
	'q','w','e','r','t','y','u','i','o','p','[',']','\n',
	0, // LCTRL
	'a','s','d','f','g','h','j','k','l',';','\'', '`',
	0, // LSHIFT
	'\\','z','x','c','v','b','n','m',',','.','/',
	0, // RSHIFT
	'*',
	0, // LALT
	' ', 
	0, // CapsLock
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1 - F10
	0, // NUMLOCK
	0, // SCROLLOCK
	0, // EX HOME
	0, // EX UP
	0, // EX PG UP
	'-',
	0, // EX L ARROW
	0,
	0, // EX R ARROW
	'+',
	0, // EX END
	0, // EX DOWN
	0, // EX PG DOWN
	0, // EX INSERT
	0, // EX DELETE
	0, 0, 0,
	0, // F11
	0, // F12 
	0, // Other
};

static const char __scancodes_shifted[128] = {
	0, 27,
	'!','@','#','$','%','^','&','*','(',')',
	'_','+','\b',
	'\t', /* tab */
	'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
	0, /* control */
	'A','S','D','F','G','H','J','K','L',':','"', '~',
	0, /* left shift */
	'|','Z','X','C','V','B','N','M','<','>','?',
	0, /* right shift */
	'*',
	0, /* alt */
	' ', /* space */
	0, /* caps lock */
	0, /* F1 [59] */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, /* ... F10 */
	0, /* 69 num lock */
	0, /* scroll lock */
	0, /* home */
	0, /* up */
	0, /* page up */
	'-',
	0, /* left arrow */
	0,
	0, /* right arrow */
	'+',
	0, /* 79 end */
	0, /* down */
	0, /* page down */
	0, /* insert */
	0, /* delete */
	0, 0, 0,
	0, /* F11 */
	0, /* F12 */
	0, /* everything else */
};

static const char __scancodes_ext[128] = {
	[0x47] = KEY_HOME,
	[0x1C] = KEY_KPENTER,
	[0x35] = KEY_KPSLASH,
	[0x4B] = KEY_LEFT,
	[0x5B] = KEY_LEFTMETA,
	[0x4D] = KEY_RIGHT
};

keyboard_packet_t* input_kbd_create_packet(int scancode) {
	int flags = 0;
	if(scancode & 0x80) {
		flags |= KBD_EVENT_FLAG_UP;
	}
	if(scancode & 0x100) {
		flags |= KBD_EVENT_FLAG_EXT;
	}
	scancode &= 0x7F;


	keyboard_packet_t* packet = calloc(1, sizeof(keyboard_packet_t));
	packet->scancode = (flags & KBD_EVENT_FLAG_EXT) ? __scancodes_ext[scancode] : scancode; 
	packet->flags    = flags;

	return packet;
}


char input_kbd_translate(int scancode, uint8_t mods) {
	if(scancode < 0 || scancode > sizeof(__scancodes)) {
		return 0;
	}

	char t = 0;

	if(mods & KBD_MOD_SHIFT) {
		t = __scancodes_shifted[scancode];	
	} else {
		t = __scancodes[scancode];
	}

	if(t && mods & KBD_MOD_CAPS) {
		t = toupper(t);
	}

	return t;
}
