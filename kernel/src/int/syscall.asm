extern __k_int_syscall_dispatcher

[global _syscall_stub]
align 4
_syscall_stub:
    cli
	push 0
	push 0x80
	pushad
    cld
    call __k_int_syscall_dispatcher
    popad
    add esp, 8
    sti
    iret