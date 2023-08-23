extern libc_init
extern libc_exit

section .text
global _start

_start:
	call libc_init

    push eax

    call libc_exit
