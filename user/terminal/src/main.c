#include "compose/compose.h"

#include "compose/events.h"
#include "fb.h"
#include "input/keys.h"
#include "sys/select.h"
#include "sys/time.h"
#include "widgets/widget.h"
#include "widgets/win.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/tty.h>

static compose_client_t* client = NULL;
static widget* window    = NULL;

int master, slave;

char*  buff      = NULL;
size_t buff_size = 0;
off_t  buff_offs = 0;

char*  input      = NULL;
size_t input_size = 0;
off_t  input_offs = 0;

int    scroll = 0;

void buff_putchar(char c, char** b, off_t* off, size_t* sz) {
	if(!*b) {
		*sz = 64;
		*b  = calloc(1, 64);
	}
	if(*off >= *sz - 1) {
		(*sz) *= 2;
		*b = realloc(*b, *sz);
		memset(*b + *off, 0, *sz - *off);
	}	
	(*b)[*off] = c;
	(*off)++;
}

void buff_refresh() {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(master, &set);

	struct timeval tv;
	tv.tv_msec = 0;
	tv.tv_sec  = 0;

	int r  = select(master + 1, &set, NULL, NULL, &tv);
	int wr = 0;

	// printf("Select returned: %d\n", r);

	while (r > 0) {
		wr = 1;
		char c;
		read(master, &c, 1);
		// printf("%c", c);
		buff_putchar(c, &buff, &buff_offs, &buff_size);
		FD_ZERO(&set);
		FD_SET(master, &set);
	 	r = select(master + 1, &set, NULL, NULL, &tv);
	}
	// fflush(stdout);

	if(wr) {
		widget_draw(window);
	}
}

void buff_draw(widget* w) {
	if(!buff) {
		return;
	}

	char line[buff_offs + input_offs + 1];
	memset(line, 0, buff_offs + input_offs + 1);

	char o = 0;

	int x = 0;
	int y = 0;

	fb_font_t* sys_font = fb_system_font();

	int cw = sys_font->width;
	int ch = sys_font->height;

	list_t* lines = list_create();

	if(buff) {
		for(int i = 0; i < buff_offs; i++) {
			char c = buff[i];
			if(c == '\n' || o * cw > w->props.size.w) {
				list_push_back(lines, strdup(line));
				o = 0;
				line[0] = '\0';
			} else {
				line[o] = c;
				line[o + 1] = '\0';
				o++;
			}
		}
	}

	if(input) {
		for(int i = 0; i < input_offs; i++) {
			char c = input[i];
			if(c == '\n' || o * cw > w->props.size.w) {
				list_push_back(lines, strdup(line));
				o = 0;
				line[0] = '\0';
			} else {
				line[o] = c;
				line[o + 1] = '\0';
				o++;
			}
		}
	}

	if(o) {
		list_push_back(lines, strdup(line));
	}

	int max_lines =  w->props.size.h / ch;

	int offs = 0;
	if(lines->size > max_lines) {
		offs = lines->size - max_lines;
	}

	int lc = lines->size - offs;
	if(lc > max_lines) {
		lc = max_lines;
	}

	for(int i = 0; i < lines->size; i++) {
		if(i >= offs && i < offs + lc) {
			fb_string(&w->ctx->fb, x, y, lines->data[i], NULL, 0x0, 0xFFFFFFFF);
			y += ch;
		}
		free(lines->data[i]);
	}

	list_free(lines);
}

void draw_terminal(widget* window) {
	fb_fill(&window->ctx->fb, 0xFF000000);
	window_draw(window);
	buff_draw(window);
}

void terminal_handle_event(widget* w, compose_event_t* ev) {
	window_process_events(w, ev);

	if(ev->type == COMPOSE_EVENT_KEY) {
		compose_key_event_t* mev = (compose_key_event_t*) ev;

		if(mev->packet.flags & KBD_EVENT_FLAG_UP) {
			return;
		}

		if(mev->packet.scancode == KEY_ENTER) {
			buff_putchar('\n', &input, &input_offs, &input_size);
			write(master, input, input_offs);
			input_offs = 0;
			input[0] = '\0';
			widget_draw(w);
		} else if(mev->packet.scancode == KEY_BACKSPACE) {
			if(!input_offs) {
				return;
			}
			input_offs--;
			input[input_offs] = '\0';
			widget_draw(w);
		} else {
			char t = mev->translated;
			if(t) {
				buff_putchar(t, &input, &input_offs, &input_size);
				widget_draw(w);
			}
		}
	}
}

int main(int argc, char** argv) {
	client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		perror("Failed to connect to compose server.");
		return 1;
	}

	widgets_init();

	widget_properties props = {0};
	props.pos.x = 500;
	props.pos.y = 300;
	props.size.w = 400;
	props.size.h = 200;
	props.padding.left  = 5;
	props.padding.right = 5;
	window = widget_create(client, WIDGET_TYPE_WINDOW, NULL, props, NULL);
	window->ops.draw = draw_terminal;
	window->ops.process_event = terminal_handle_event;

	compose_cl_evmask(client, window->win, COMPOSE_EVENT_KEY);

	int r = openpty(&master, &slave, NULL, NULL, NULL);
	if(r < 0) {
		perror("Failed to open pty");
		return 1;
	}
	
	pid_t child = fork();
	if(!child) {
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		execve("/bin/scsh", NULL, NULL);
		exit(1);
	}

	widget_draw(window);

	while(1) {
		buff_refresh();
		widgets_tick(window);
	}

	return 0;

}
