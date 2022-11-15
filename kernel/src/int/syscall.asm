extern __k_int_syscall_dispatcher

[global _syscall_stub]
align 4
_syscall_stub:
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