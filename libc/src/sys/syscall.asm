[global __syscall]
__syscall:
    push ebp
    mov  ebp, esp

	push ebx
	push esi
	push edi

    mov eax, [ebp + 8]
    
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20]
    mov edi, [ebp + 24]
    mov esi, [ebp + 28]

    int 0x80

	pop edi
	pop esi
	pop ebx

	leave
    ret
