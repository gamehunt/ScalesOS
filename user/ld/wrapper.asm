extern resolve_symbol
global __resolver_wrapper
__resolver_wrapper:
	call resolve_symbol
	add  esp, 0x8
	push eax
	ret

global __jump:
__jump:
	mov  eax, [esp + 4]
	add  esp, 8
	push eax
	ret

