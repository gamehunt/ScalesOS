extern main
extern exit

section .text
global _start

_start:
    mov  ebp, 0
	
    push ebp
    push ebp

	mov  ebp, esp

    call main

    push eax

    call exit
