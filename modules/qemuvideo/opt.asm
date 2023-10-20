[global __fast_memset]
__fast_memset:
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

[global __fast_memcpy]
__fast_memcpy:
	push ebp
	mov  ebp, esp
	cld
	push edi
	push esi
	mov edi, [ebp + 8]
	mov esi, [ebp + 12]
	mov ecx, [ebp + 16]
	rep movsd
	mov eax, edi
	pop esi
	pop edi
	leave
	ret
