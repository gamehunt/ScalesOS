%macro setup_context 0
    mov edx, [ebx + 12] ; Load new page directory if we need it
    
    push 0
    push 1
    push edx
    call k_mem_paging_set_pd
    pop edx
    add esp, 8

    mov ebp, [ebx + 4] ; Restore EBP
    mov esp, [ebx + 0] ; Restore ESP
%endmacro

[global __k_proc_process_save]
__k_proc_process_save:
    mov edi, [esp + 4] ; Get arg

    mov [edi + 0], esp ; Save ESP
    mov [edi + 4], ebp ; Save EBP

    mov eax, [esp]     ; Get EIP
    mov [edi + 8], eax ; Save EIP

    xor eax, eax       ; return 0
    ret

extern k_mem_paging_set_pd

[global __k_proc_process_load]
__k_proc_process_load:
    mov ebx, [esp + 4] ; Get arg
    
    setup_context

    mov  eax, 1    ; Make save function return 1
    jmp [ebx + 8]  ; Jump

[global __k_proc_process_enter_usermode]
__k_proc_process_enter_usermode:
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]

    setup_context

	mov ax, 0x20 | 3 ; Ring 3 data
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax
 
	push 0x20 | 3 ; SS
	push ecx      ; ESP
	
    pushf
    pop  eax
    or   eax, 0x200
    push eax ; Push eflags with interrupts enabled

    push 0x18 | 3  ; code selector (ring 3 code with bottom 2 bits set for ring 3)

    mov  eax, [ebx + 8]
	push eax ; instruction address to return to

	iret