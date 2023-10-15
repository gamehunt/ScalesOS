#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <scales/env.h>

extern void __mem_init_heap();
extern void __init_stdio();
extern int  main(int argc, char** argv);
extern void _init();
extern void _fini();

char** environ;
int    __envc = 0;

char** __argv;
int    __argc = 0;

struct env_var** __env;

static void __parse_env() {
	if(!__envc){
		return;
	}
	__env = malloc(__envc * sizeof(struct env_var*));
	int i = 0;
	while(i < __envc) {
		struct env_var* var = malloc(sizeof(struct env_var));

		char* name = strtok(environ[i], "=");
		if(!name) {
			var->name  = environ[i];
			var->value = "\0";
		} else {
			var->name  = name;
			var->value = strtok(NULL, "=");
		}
		
		__env[i] = var;
		i++;
	}
}

void __init_arguments(int argc, char** argv, int envc, char** envp) {
	__argc = argc;
	__argv = malloc((__argc + 1) * sizeof(char*));

	__envc = envc;
	environ = malloc((__envc + 1) * sizeof(char*));

	if(__argc) {
		for(int i = 0; i < __argc; i++) {
			__argv[i] = malloc(strlen(argv[i]) + 1);
			strcpy(__argv[i], argv[i]);
		}
	}
	__argv[__argc] = "\0";

	if(__envc) {
		for(int i = 0; i < __envc; i++) {
			environ[i] = malloc(strlen(envp[i]) + 1);
			strcpy(environ[i], envp[i]);
		}
	}
	environ[__envc] = "\0";
}

void __atexit_flush_streams() {
	fflush(stderr);
	fflush(stdout);
}

void libc_exit(int code) {
	_fini();
	exit(code);
}

extern void _debug(int mode);

void libc_init(int argc, char** argv, int envc, char** envp) {
	__mem_init_heap();
	__init_stdio();
	__init_arguments(argc, argv, envc, envp);
	__parse_env();
	_init();
	atexit(&__atexit_flush_streams);
	libc_exit(main(__argc, __argv));
}

void align_fail() {
	exit(0xEE);	
}


