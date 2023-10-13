#include "util/exec.h"
#include "errno.h"
#include "fs/vfs.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mod/elf.h"
#include "proc/process.h"
#include "util/log.h"
#include "util/types/list.h"
#include "util/types/stack.h"

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


int k_util_exec(const char* path, const char* argv[], const char* envp[]) {
	fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);

	if(!node) {
		return -ENOENT;
	}

	uint8_t buffer[4];

	k_fs_vfs_read(node, 0, 4, buffer);
	k_fs_vfs_close(node);

	for(uint32_t i = 0; i < exec_list->size; i++){
		executable_t* fmt = exec_list->data[i];
		if(!memcmp(buffer, fmt->signature, fmt->length)) {
			int argc = 0;
			if(argv) {
				char** ptr = argv;
				while(*ptr) {
					ptr++;
					argc++;
				}

				char** copied_argv = k_malloc(argc * sizeof(char*));

				ptr = argv;
				char** ptr1 = copied_argv;
				while(*ptr) {
					*ptr1 = k_malloc(strlen(*ptr) + 1);
					strcpy(*ptr1, *ptr);
					ptr++;
					ptr1++;
				}

				argv = copied_argv;
			} 
			return fmt->exec_func(path, argc, argv, envp);
		}
	}

	return -EINVAL;
}

extern __attribute__((noreturn)) void __k_proc_process_enter_usermode(uint32_t entry, uint32_t userstack);
int k_util_exec_elf(const char* path, int argc, const char* argv[], const char* envp[]) {
	fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);
	if(!node) {
		return -ENOENT;
	}

    uint8_t* buffer = k_malloc(node->size);
    uint32_t read = k_fs_vfs_read(node, 0, node->size, buffer);
	if(read != node->size) {
		k_err("Failed to read whole file.");
    	k_fs_vfs_close(node);
		k_free(buffer);
		return -EIO;
	}


	Elf32_Ehdr* hdr = (Elf32_Ehdr*) buffer;

	if(!k_mod_elf_check(hdr) || hdr->e_type != ET_EXEC) {
    	k_fs_vfs_close(node);
		k_free(buffer);
		return -ENOEXEC;
	}

    Elf32_Phdr* phdr = (Elf32_Phdr*)((uint32_t)hdr + hdr->e_phoff);

	char*   interp = NULL;
	uint8_t dynamic = 0;
	for(uint32_t i = 0; i < hdr->e_phnum; i++) {
		if(phdr->p_type == PT_INTERP) {
			interp = malloc(phdr->p_memsz);
			memcpy(interp, hdr + phdr->p_offset, phdr->p_filesz);
		} else if(phdr->p_type == PT_DYNAMIC) {
			dynamic = 1;
			break;
		}
		phdr = (Elf32_Phdr*)((uint32_t)phdr + hdr->e_phentsize);
	}

	if(dynamic) {
		k_free(buffer);
    	k_fs_vfs_close(node);

		if(!interp || !strlen(interp)) {
			interp = "/lib/ld.so";
		}

		int new_argc = argc + 1;
		char** new_argv = NULL;
		if(!argv) {
			new_argv = malloc(sizeof(char*) * 2);
			new_argv[1] = 0;
		} else {
			new_argv = realloc(argv, new_argc * sizeof(char*));
			memmove(new_argv + 1, new_argv, argc * sizeof(char*));
		}

		new_argv[0] = path;

		k_debug("DYN: Executing: %s %s", interp, path);

		return k_util_exec(interp, new_argv, envp);
	}
	
    process_t* proc = k_proc_process_current(); 
    strcpy(proc->name, node->name);
    	
	k_fs_vfs_close(node);

	pde_t* old = proc->image.page_directory;
	proc->image.page_directory = k_mem_paging_clone_root_page_directory(NULL);
	k_mem_paging_set_page_directory((pde_t*) proc->image.page_directory, 0); 																
	k_mem_paging_release_directory(old);

	proc->image.heap       = USER_HEAP_START;
	proc->image.heap_size  = USER_HEAP_INITIAL_SIZE;
	proc->image.user_stack = USER_STACK_START + USER_STACK_SIZE;

    k_mem_paging_map_region(USER_STACK_START, 0, USER_STACK_SIZE / 0x1000, 0x7, 0);
	k_mem_paging_map_region(USER_HEAP_START,  0, USER_HEAP_INITIAL_SIZE / 0x1000, 0x7, 0);

	phdr = (Elf32_Phdr*)((uint32_t)hdr + hdr->e_phoff);
    for (uint32_t i = 0; i < hdr->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            k_mem_paging_map_region(phdr->p_vaddr & 0xFFFFF000, 0,
                                    phdr->p_memsz / 0x1000 + 1, 0x7, 0x0);
            memset((void*)phdr->p_vaddr, 0, phdr->p_memsz);
            memcpy((void*)phdr->p_vaddr,
                   (void*)((uint32_t)hdr + phdr->p_offset), phdr->p_filesz);
        }
        phdr = (Elf32_Phdr*)((uint32_t)phdr + hdr->e_phentsize);
    }

	uint32_t entry = hdr->e_entry;

	k_free(buffer);

	int       envc       = 0;
	uintptr_t envp_stack = 0; 

	if(envp) {
		while(envp[envc]) {
			envc++;
		}

		if(envc) {
			char** ptrs = k_calloc(envc, sizeof(char*));
			int j = 0;
			for(int env = envc - 1; env >= 0; env--) {
				PUSH(proc->image.user_stack, char, '\0');
				for(int i = strlen(envp[env]) - 1; i >= 0; i--) {
					PUSH(proc->image.user_stack, char, envp[env][i]);
				}
				ptrs[j] = (char*) proc->image.user_stack;
				j++;
			}

			for(int i = 0; i < envc; i++) {
				PUSH(proc->image.user_stack, char*, ptrs[i]);
			}
			
			k_free(ptrs);

			char** _envp = (char**) proc->image.user_stack;
			envp_stack = (uintptr_t) _envp;
		} 
	}		

	uintptr_t argv_stack = 0;
	argc++;

	if(!argv) {
		argv = malloc(sizeof(char*));
	} else {
		argv = realloc(argv, sizeof(char*) * (argc));
		memmove(argv + 1, argv, sizeof(char*) * (argc - 1));
	}

	argv[0] = malloc(strlen(proc->name) + 1);
	strcpy(argv[0], proc->name);

	char** ptrs = k_calloc(argc, sizeof(char*)); 
	for(int i = argc - 1; i >= 0; i--) {
		PUSH(proc->image.user_stack, char, '\0');
		for(int j = strlen(argv[i]) - 1; j >= 0; j--) {
			PUSH(proc->image.user_stack, char, argv[i][j])		
		}
		ptrs[i] = (char*) proc->image.user_stack;
	}

	k_free(argv);
    
	for(int i = argc - 1; i >= 0; i--) {
		PUSH(proc->image.user_stack, char*, ptrs[i])
	}
    
	k_free(ptrs);

	char** _argv = (char**) proc->image.user_stack;
	argv_stack = (uintptr_t) _argv;

	while(proc->image.user_stack % 16) {
		PUSH(proc->image.user_stack, char, '\0');
	} 

	PUSH(proc->image.user_stack, uintptr_t, envp_stack);
	PUSH(proc->image.user_stack, int,       envc);
	PUSH(proc->image.user_stack, uintptr_t, argv_stack);
	PUSH(proc->image.user_stack, int,       argc);
	
	// uint32_t occupied_stack_size = USER_STACK_START + USER_STACK_SIZE - proc->image.user_stack;
	// for(uint32_t i = 0; i < occupied_stack_size; i += 4) {
	// 	k_debug("[0x%.8x] 0x%.8x", proc->image.user_stack + i, *((uint32_t*) (proc->image.user_stack + i)));
	// }
	
	proc->image.entry = entry;

    k_mem_gdt_set_stack((uint32_t) proc->image.kernel_stack);

    __k_proc_process_enter_usermode(entry, proc->image.user_stack);

}

int k_util_exec_shebang(const char* path, int argc, const char* argv[], const char* envp[]) {
	fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);
	if(!node) {
		return -ENOENT;
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

	k_debug("Interpeter: %s", interp);

	if(!argv) {
		argv = k_malloc(sizeof(char*));
		argv[0] = 0;
	}

	argv = realloc(argv, (argc + 2) * sizeof(char*));
	argv[argc] = malloc(strlen(path) + 1);
	strcpy(argv[argc], path);
	argv[argc + 1] = 0;

	return k_util_exec(interp, argv, envp);
}
