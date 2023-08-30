extern libc_init
extern align_fail

section .text
global _start

_start:
	mov ebx, esp
	and ebx, 0xFFFFFFF0
	mov ecx, ebx
	sub ebx, esp
	jnz no_alignment
	call libc_init
no_alignment:
	call align_fail
