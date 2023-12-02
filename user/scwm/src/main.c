#include "compose/compose.h"
#include "compose/events.h"
#include "compose/render.h"

#include "input/keys.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {	
	if(!fork()) {
		execve("/bin/terminal", NULL, NULL);
		exit(1);
	}

	compose_client_t* client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		return -1;
	}

	compose_cl_gc_t* root_gc    = compose_cl_get_gc(client->root);
	compose_cl_gc_t* overlay_gc = compose_cl_get_gc(client->overlay);

	fb_fill(&root_gc->fb, 0xFF000000);
	compose_cl_grab(client, client->root, COMPOSE_EVENT_KEY | COMPOSE_EVENT_BUTTON | COMPOSE_EVENT_MOUSE | COMPOSE_EVENT_WIN);

	int mod = 0;

	int mov = 0;
	int res = 0;

	int xres_dir = 0;
	int yres_dir = 0;

	window_properties_t root_props = compose_cl_get_properties(client, client->root);
	window_properties_t win_props       = {0};

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client, 1);
		if(ev) {
			switch(ev->type) {
				case COMPOSE_EVENT_WIN:
					compose_cl_move(client, ev->child, 0, 0);
					compose_cl_resize(client, ev->child, root_props.w, root_props.h);
					break;
				case COMPOSE_EVENT_KEY:
					if(((compose_key_event_t*) ev)->packet.scancode == KEY_LEFTMETA) {
						mod = !(((compose_key_event_t*) ev)->packet.flags & KBD_EVENT_FLAG_UP);
						if(!mod) {
							window_properties_t old_props = win_props;
							int wr = res;
							res = 0;
							mov = 0; 
							if(wr) {
								fb_blend_mode(&overlay_gc->fb, FB_NO_BLEND);
								fb_rect(&overlay_gc->fb, old_props.x, old_props.y, old_props.w, old_props.h, 0x00000000);
								fb_blend_mode(&overlay_gc->fb, FB_BLEND_DEFAULT);
								compose_cl_resize(client, wr, old_props.w, old_props.h);
								compose_cl_move(client, wr, old_props.x, old_props.y);
							}
						}
					}
					break;
				case COMPOSE_EVENT_BUTTON:
					if(ev->child) {
						compose_cl_focus(client, ev->child);
						compose_cl_raise(client, ev->child);
					}
					if(mod && ev->child) {
						window_properties_t old_props = win_props;
						win_props = compose_cl_get_properties(client, ev->child);
						int wr = res;
						if(((compose_mouse_event_t*) ev)->packet.buttons & MOUSE_BUTTON_LEFT) {
							mov = ev->child;
							res = 0;
							if(wr) {
								fb_blend_mode(&overlay_gc->fb, FB_NO_BLEND);
								fb_rect(&overlay_gc->fb, old_props.x, old_props.y, old_props.w, old_props.h, 0x00000000);
								fb_blend_mode(&overlay_gc->fb, FB_BLEND_DEFAULT);
						 		compose_cl_resize(client, wr, old_props.w, old_props.h);
								compose_cl_move(client, wr, old_props.x, old_props.y);
							}
						} else {
							mov = 0;
							if(((compose_mouse_event_t*) ev)->packet.buttons & MOUSE_BUTTON_RIGHT) {
								res = ev->child;
								int posx = ((compose_mouse_event_t*) ev)->x;
								int posy = ((compose_mouse_event_t*) ev)->y;

								if(posx > win_props.w / 2) {
									xres_dir = 1;
								} else {
									xres_dir = 0;
								}

								if(posy > win_props.h / 2) {
									yres_dir = 1;
								} else {
									yres_dir = 0;
								}
							} else {
								res = 0;
								if(wr) {
									fb_blend_mode(&overlay_gc->fb, FB_NO_BLEND);
									fb_rect(&overlay_gc->fb, old_props.x, old_props.y, old_props.w, old_props.h, 0x00000000);
									fb_blend_mode(&overlay_gc->fb, FB_BLEND_DEFAULT);
						 			compose_cl_resize(client, wr, old_props.w, old_props.h);
									compose_cl_move(client, wr, old_props.x, old_props.y);
								}
							}
						}
					}
					break;
				case COMPOSE_EVENT_MOUSE:
					if(mov) {
						int dx = ((compose_mouse_event_t*) ev)->packet.dx;
						int dy = ((compose_mouse_event_t*) ev)->packet.dy;

						win_props.x += dx;
						win_props.y -= dy;
							
						compose_cl_move(client, mov, win_props.x, win_props.y);
					} else if(res) {
						window_properties_t prev = win_props;

						if(xres_dir) {
							win_props.w += ((compose_mouse_event_t*) ev)->packet.dx;
						} else {
							win_props.w -= ((compose_mouse_event_t*) ev)->packet.dx;
							win_props.x += ((compose_mouse_event_t*) ev)->packet.dx;
						}

						if(yres_dir) {
							win_props.h -= ((compose_mouse_event_t*) ev)->packet.dy;
						} else {
							win_props.h += ((compose_mouse_event_t*) ev)->packet.dy;
							win_props.y -= ((compose_mouse_event_t*) ev)->packet.dy;
						}

						if(win_props.w < 10) {
							win_props.w = 10;
						}

						if(win_props.h < 10) {
							win_props.h = 10;
						}
						// compose_cl_resize(client, res, win_props.w, win_props.h);
						fb_blend_mode(&overlay_gc->fb, FB_NO_BLEND);
						fb_rect(&overlay_gc->fb, prev.x, prev.y, prev.w, prev.h, 0x00000000);
						fb_blend_mode(&overlay_gc->fb, FB_BLEND_DEFAULT);
						fb_rect(&overlay_gc->fb, win_props.x, win_props.y, win_props.w, win_props.h, 0xFF0000FF);
						compose_cl_flush(client, client->overlay);
					}
					break;
			}
			free(ev);
		}
	}

	return 0;
}
