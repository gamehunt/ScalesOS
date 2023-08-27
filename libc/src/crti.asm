global _init
section .init
_init:
	push ebp
	mov  ebp, esp

global _fini
section .fini
_fini:
	push ebp
	mov  ebp, esp
