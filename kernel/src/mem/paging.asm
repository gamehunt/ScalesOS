[global __k_mem_paging_get_pd_phys]
__k_mem_paging_get_pd_phys:
    mov  eax, cr3
    ret

[global __k_mem_paging_set_pd]
__k_mem_paging_set_pd:
    push ebp
    mov  ebp, esp
    mov  eax, [esp + 8]
    mov  cr3, eax
    mov  esp, ebp
    pop  ebp
    ret

[global k_mem_paging_get_fault_addr]
k_mem_paging_get_fault_addr:
    mov  eax, cr2
    ret

[global jump_usermode]
jump_usermode:
    mov ebx, [esp + 4]

	mov ax, 0x20 | 3 ; ring 3 data with bottom 2 bits set for ring 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax
 
	push 0x20 | 3
	push 0x9100000
	pushf
	push 0x18 | 3 ; code selector (ring 3 code with bottom 2 bits set for ring 3)
	push ebx ; instruction address to return to
	iret