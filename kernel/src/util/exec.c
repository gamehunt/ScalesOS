#include "util/exec.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "util/log.h"
#include "util/types/list.h"

#include <stdio.h>
#include <string.h>

list_t* exec_list = 0;

void k_util_add_exec_format(executable_t exec) {
	if(!exec_list) {
		exec_list = list_create();
	}

	executable_t* copy = malloc(sizeof(exec));
	memcpy(copy, &exec, sizeof(exec));

	list_push_back(exec_list, copy);
}

static uint8_t __k_util_match_signature(uint8_t* buffer, uint8_t length, uint8_t signature[]) {
	for(uint32_t i = 0; i < length; i++){
		if(buffer[i] != signature[i]) {
			return 0;
		}
	}
	return 1;
}

int k_util_exec(const char* path, const char* argv[], const char* envp[]) {
	fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);

	if(!node) {
		return -1;
	}

	uint8_t buffer[4];

	k_fs_vfs_read(node, 0, 4, buffer);
	k_fs_vfs_close(node);

	for(uint32_t i = 0; i < exec_list->size; i++){
		executable_t* fmt = exec_list->data[i];
		if(__k_util_match_signature(buffer, fmt->length, fmt->signature)) {
			return fmt->exec_func(path, argv, envp);
		}
	}

	return -2;
}

int k_util_exec_shebang(const char* path, const char* argv[], const char* envp[]) {
	fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);
	if(!node) {
		return -1;
	}

	char interp[100];

	k_fs_vfs_read(node, 2, 100, interp);
	k_fs_vfs_close(node);

	int iter = 0;
	while(iter < 100) {
		if(interp[iter] == '\n') {
			interp[iter] = '\0';
		}
		iter++;
	}

	if(iter >= 99) {
		interp[99] = '\0';
	} 

	k_info("Interpeter: %s", interp);

	if(!argv) {
		argv = malloc(sizeof(char*));
		argv[0] = 0;
	}

	int    _argc = 0;
	char** _argv = argv;
	while(*_argv) {
		_argv++;
		_argc++;
	}


	char** new_argv = malloc(sizeof(char*) * (_argc + 2));
	if(_argc) {
		memcpy(new_argv, argv, _argc * sizeof(char*));
	}
	new_argv[_argc] = malloc(strlen(path) + 1);
	strcpy(new_argv[_argc], path);
	new_argv[_argc + 1] = 0;

	return k_proc_process_exec(interp, new_argv, envp);
}
