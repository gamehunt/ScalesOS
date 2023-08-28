extern libc_init

section .text
global _start

_start:
	call libc_init
