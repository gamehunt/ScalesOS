#include "scales/prctl.h"
#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

extern void __mem_init_heap();
extern void __init_stdio();
extern int  main(int argc, char** argv);
extern void _init();
extern void _fini();

char** environ;
int    _envc = 0;

char** __argv;
int    __argc = 0;

void __init_arguments(int argc, char** argv, int envc, char** envp) {
	__argc = argc;
	__argv = malloc((__argc + 1) * sizeof(char*));

	_envc = envc + 1;
	environ = malloc(_envc * sizeof(char*));

	if(__argc) {
		for(int i = 0; i < __argc; i++) {
			__argv[i] = strdup(argv[i]);
		}
	}
	__argv[__argc] = 0;

	if(envc) {
		for(int i = 0; i < envc; i++) {
			environ[i] = strdup(envp[i]);
		}
	}
	environ[envc] = 0;
}

void __flush_streams() {
	fflush(stderr);
	fflush(stdout);
}

void libc_exit(int code) {
	_fini();
	__flush_streams();
	exit(code);
}

extern void __init_tls();

void libc_init(int argc, char** argv, int envc, char** envp) {
	__mem_init_heap();
	__init_tls();
	__init_stdio();
	__init_arguments(argc, argv, envc, envp);
	_init();
	prctl(PRCTL_SETNAME, argv[0]);
	libc_exit(main(__argc, __argv));
}
