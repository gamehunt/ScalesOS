extern __k_int_irq_dispatcher

%macro irq 1
	[global _irq%1]
    align 4
	_irq%1:
		push 0
		push %1
		jmp irq_stub
%endmacro

irq 0
irq 1
irq 2
irq 3
irq 4
irq 5
irq 6
irq 7
irq 8
irq 9
irq 10
irq 11
irq 12
irq 13
irq 14
irq 15

irq_stub:
    pushad
    cld
    mov edi, esp
    push edi
    call __k_int_irq_dispatcher
    pop edi
    mov esp, eax
    popad
    add esp, 8
    iret