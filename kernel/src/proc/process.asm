[global __k_proc_process_save]
__k_proc_process_save:
    mov edi, [esp + 4] ; Get arg

    mov [edi + 0], esp ; Save ESP
    mov [edi + 4], ebp ; Save EBP

    mov eax, [esp]     ; Get EIP
    mov [edi + 8], eax ; Save EIP

    xor eax, eax       ; return 0
    ret

[global __k_proc_process_load]
__k_proc_process_load:
    mov ebx, [esp + 4] ; Get arg

    mov esp, [ebx + 0] ; Restore ESP
    mov ebp, [ebx + 4] ; Restore EBP

    mov  eax, 1    ; Make save function return 1
    jmp [ebx + 8]  ; Jump

[global __k_proc_process_enter_usermode]
__k_proc_process_enter_usermode:
    mov ebx, [esp + 4] ; entry
    mov ecx, [esp + 8] ; userstack

	mov ax, 0x20 | 3 ; Ring 3 data
	mov ds, ax
	mov es, ax
	mov fs, ax

	push 0x20 | 3 ; SS
	push ecx      ; ESP

    pushf
    pop  eax
    or   eax, 0x200
    push eax ; Push eflags with interrupts enabled

    push 0x18 | 3  ; code selector (ring 3 code with bottom 2 bits set for ring 3)

	push ebx ; instruction address to return to

	iret

[global __k_proc_process_fork_return]
__k_proc_process_fork_return:
    popad
    add esp, 8
    iret

extern k_proc_process_current
extern k_proc_process_exit

[global __k_proc_process_enter_tasklet]
__k_proc_process_enter_tasklet:
	pushf
	pop eax
	or eax, 0x200
	push eax
	popf
	pop  eax
	call eax
	call k_proc_process_current
	push 0
	push eax
	call k_proc_process_exit

