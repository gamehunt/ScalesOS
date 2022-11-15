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