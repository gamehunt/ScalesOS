extern libc_init

section .text
global _start

_start:
	and  esp, 0xFFFFFFFC
	call libc_init
