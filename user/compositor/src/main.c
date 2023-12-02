#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <unistd.h>
#include <input/mouse.h>

#include <compose/compose.h>

static const char* startup_app = NULL;
static char** startup_args = NULL;
static int vt = 0;

void parse_args(int argc, char** argv) {
	if(argc < 2) {
		return;
	}
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--startup")) {
			if(i + 1 < argc) {
				startup_app = argv[i + 1];
				i++;
			}
		}
		else if(!strcmp(argv[i], "--")) {
			if(i + 1 < argc) {
				startup_args = argv + i + 1;
				break;
			}
		}
	}
}

int paused = 0;

void save(int sig) {
	paused = 1;
	ioctl(vt, VT_RELDISP, (void*) 1);
	while(paused) {;}
}

void load(int sig) {
	paused = 0;
	ioctl(vt, VT_RELDISP, (void*) 0);
}

int main(int argc, char** argv) {	
	parse_args(argc, argv);

	vt = open("/dev/vt7", O_RDONLY | O_CLOEXEC);
	if(vt < 0) {
		return -1;
	}

	int tty = open("/dev/tty7", O_RDWR | O_CLOEXEC);
	if(tty < 0) {
		return -1;
	}
	tcsetpgrp(tty, getpid());

	ioctl(vt, VT_ACTIVATE, NULL);
	struct vt_mode mode;
	mode.mode   = VT_MODE_PROCESS;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR2;
	ioctl(vt, VT_SETMODE, &mode);
	ioctl(vt, KDSETMODE, (void*) VT_DISP_MODE_GRAPHIC);

	signal(SIGUSR1, save);
	signal(SIGUSR2, load);
	signal(SIGINT, SIG_IGN);

	printf("Starting server...\n");

	remove("/tmp/.compose.sock");
	compose_server_t* srv = compose_sv_create("/tmp/.compose.sock");

	if(startup_app) {
		if(!fork()) {
			execve(startup_app, startup_args, NULL);
			exit(-1);
		}
	}

	fb_fill(&srv->framebuffer, 0xFF000000);
	fb_flush(&srv->framebuffer);

	printf("Server started.\n");
	
	while(1) {
		compose_sv_tick(srv);
	}

	return 0;
}
