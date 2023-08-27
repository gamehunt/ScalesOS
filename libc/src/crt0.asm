extern libc_init

section .text
global _start

_start:
	and  esp, 0xFFFFFFFF0
	call libc_init
