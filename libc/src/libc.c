#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/__internals.h>

extern void __mem_init_heap();
extern int main(int argc, char** argv, char** envp);

char** __envp = {"\0"};
int    __envc = 0;

struct env_var** __env;

static void __parse_env() {
	if(!__envc){
		return;
	}
	__env = malloc(__envc * sizeof(struct env_var*));
	int i = 0;
	while(i < __envc) {
		struct env_var* var = malloc(sizeof(struct env_var));

		char* name = strtok(__envp[i], "=");
		if(!name) {
			var->name  = __envp[i];
			var->value = "\0";
		} else {
			var->name  = name;
			var->value = strtok(NULL, "=");
		}
		
		__env[i] = var;
		i++;
	}
}


int libc_init(int argc, char** argv, int envc, char** envp) {
	__mem_init_heap();

	char** new_argv = {"\0"};
	__envc = envc;

	if(argc) {
		new_argv = malloc(argc * sizeof(char*));
		for(int i = 0; i < argc; i++) {
			new_argv[i] = malloc(strlen(argv[i]) + 1);
			strcpy(new_argv[i], argv[i]);
		}
	}

	if(__envc) {
		__envp = malloc((__envc + 1) * sizeof(char*));
		for(int i = 0; i < __envc; i++) {
			__envp[i] = malloc(strlen(envp[i]) + 1);
			strcpy(__envp[i], envp[i]);
		}
		__envp[__envc] = "\0";
	}

	__parse_env();

	return main(argc, new_argv, __envp);
}

void libc_exit(int code) {
	exit(code);
}
