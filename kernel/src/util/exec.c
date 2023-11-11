#include "util/exec.h"
#include "errno.h"
#include "fs/vfs.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mem/mmap.h"
#include "mem/paging.h"
#include "mod/elf.h"
#include "proc/process.h"
#include "util/log.h"
#include "types/list.h"
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


int k_util_exec(const char* path, int argc, const char* argv[], const char* envp[]) {
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
			memcpy(interp, (void*) (((uint32_t)hdr) + phdr->p_offset), phdr->p_filesz);
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

		int new_argc = argc + 2;
		const char* new_argv[new_argc];
		new_argv[0] = path;
		new_argv[1] = path;
		for(int i = 0; i < argc; i++) {
			new_argv[i + 2] = argv[i];
		}

		// k_debug("DYN: Executing: %s %s", interp, path);

		return k_util_exec(interp, new_argc, new_argv, envp);
	}
	
    process_t* proc = k_proc_process_current(); 
    strcpy(proc->name, node->name);

	argc++;

	const char** _argv = k_calloc(argc, sizeof(char*));
	_argv[0] = proc->name; 
	for(int i = 0; i < argc - 1; i++) {
		_argv[i + 1] = strdup(argv[i]);
	}
    	
	k_fs_vfs_close(node);

	while(proc->image.mmap_info.mmap_blocks->size > 0) {
		mmap_block_t* block = list_pop_back(proc->image.mmap_info.mmap_blocks);
		k_mem_mmap_free_block(&proc->image.mmap_info, block);
	}

	pde_t* old = proc->image.page_directory;
	proc->image.page_directory = k_mem_paging_clone_root_page_directory(NULL);
	k_mem_paging_set_page_directory((pde_t*) proc->image.page_directory, 0); 																
	k_mem_paging_release_directory(old);

	uint32_t executable_end = 0x0;

	phdr = (Elf32_Phdr*)((uint32_t)hdr + hdr->e_phoff);
    for (uint32_t i = 0; i < hdr->e_phnum; i++) {
        if (phdr->p_type == PT_LOAD) {
            k_mem_paging_map_region(phdr->p_vaddr & 0xFFFFF000, 0,
                                    phdr->p_memsz / 0x1000 + 1, 0x7, 0x0);
            memset((void*)phdr->p_vaddr, 0, phdr->p_memsz);
            memcpy((void*)phdr->p_vaddr,
                   (void*)((uint32_t)hdr + phdr->p_offset), phdr->p_filesz);
			if(executable_end < phdr->p_vaddr + phdr->p_memsz) {
				executable_end = phdr->p_vaddr + phdr->p_memsz;
			}
        }
        phdr = (Elf32_Phdr*)((uint32_t)phdr + hdr->e_phentsize);
    }

	executable_end = ALIGN(executable_end, 0x1000);
	executable_end += 0x1000;

	k_mem_paging_map_region(USER_STACK_END, 0, USER_STACK_SIZE / 0x1000, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER, 0);

	proc->image.user_stack = USER_STACK_START;

	proc->image.heap       = executable_end;
	proc->image.heap_size  = USER_HEAP_INITIAL_SIZE;

	if(USER_MMAP_START > proc->image.heap + USER_HEAP_INITIAL_SIZE) {
		proc->image.mmap_info.start = USER_MMAP_START;
	} else {
		proc->image.mmap_info.start = proc->image.heap + USER_HEAP_INITIAL_SIZE;
	}


	k_mem_paging_map_region(proc->image.heap, 0, proc->image.heap_size / 0x1000, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER, 0);
	uint32_t entry = hdr->e_entry;

	k_free(buffer);

	int       envc       = 0;
	uintptr_t envp_stack = 0; 

	if(envp) {
		while(envp[envc]) {
			envc++;
		}

		if(envc) {
			char* ptrs[envc];
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
			
			envp_stack = (uintptr_t) proc->image.user_stack;
		} 
	}		

	uintptr_t argv_stack = 0;

	char* ptrs[argc];  
	for(int i = argc - 1; i >= 0; i--) {
		PUSH(proc->image.user_stack, char, '\0');
		for(int j = strlen(_argv[i]) - 1; j >= 0; j--) {
			PUSH(proc->image.user_stack, char, _argv[i][j])		
		}
		ptrs[i] = (char*) proc->image.user_stack;
		if(i) {
			k_free(_argv[i]);
		}
	}

	k_free(_argv);

	for(int i = argc - 1; i >= 0; i--) {
		PUSH(proc->image.user_stack, char*, ptrs[i])
	}
    
	argv_stack = proc->image.user_stack;

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

	const char* _argv[argc + 1];
	char _path[strlen(path) + 1];
	strcpy(_path, path);

	_argv[0] = _path;
	for(int i = 0; i < argc; i++) {
		_argv[i + 1] = argv[i];
	}

	return k_util_exec(interp, argc + 1, _argv, envp);
}
