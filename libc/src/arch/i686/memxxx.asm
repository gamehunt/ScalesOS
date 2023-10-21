[global __arch_memcpy]
__arch_memcpy:
	push ebp
	mov  ebp, esp
	cld
	push edi
	push esi
	mov edi, [ebp + 8]
	mov esi, [ebp + 12]
	mov ecx, [ebp + 16]
	rep movsb
	mov eax, edi
	pop esi
	pop edi
	leave
	ret

[global __arch_memset]
__arch_memset:
	push ebp
	mov  ebp, esp
	cld
	push edi
	mov edi, [ebp + 8]
	mov eax, [ebp + 12]
	mov ecx, [ebp + 16]
	rep stosb
	mov eax, edi
	pop edi
	leave
	ret

[global __memset32]
__memset32:
	push ebp
	mov  ebp, esp
	cld
	push edi
	mov edi, [ebp + 8]
	mov eax, [ebp + 12]
	mov ecx, [ebp + 16]
	rep stosd
	mov eax, edi
	pop edi
	leave
	ret

[global __memset16]
__memset16:
	push ebp
	mov  ebp, esp
	cld
	push edi
	mov edi, [ebp + 8]
	mov eax, [ebp + 12]
	mov ecx, [ebp + 16]
	rep stosw
	mov eax, edi
	pop edi
	leave
	ret
