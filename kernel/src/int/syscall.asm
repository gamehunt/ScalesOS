extern __k_int_syscall_dispatcher

%macro fix_gs 0
    push eax
    mov ax, 0x38
    mov gs, ax
    pop eax
%endmacro

[global _syscall_stub]
align 4
_syscall_stub:
    fix_gs
    push 0
	push 0x80
	pushad
    cld
    mov edi, esp
    push edi
    call __k_int_syscall_dispatcher
    pop edi
    mov esp, eax
    popad
    add esp, 8
    iret
