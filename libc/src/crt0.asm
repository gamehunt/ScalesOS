extern main
extern libc_init
extern libc_exit

section .text
global _start

_start:
    mov  ebp, 0
	
    push ebp
    push ebp

	mov  ebp, esp

	call libc_init
    call main

    push eax

    call libc_exit
