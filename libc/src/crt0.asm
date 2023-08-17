extern main
extern libc_init
extern libc_exit

section .text
global _start

_start:
    push ebp
	mov  ebp, esp

	call libc_init

	mov  ecx, [ebp + 4] ; argc
	mov  edx, [ebp + 8] ; argv

	push edx
	push ecx

	call main

    push eax

    call libc_exit
