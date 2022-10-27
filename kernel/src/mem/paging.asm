[global k_mem_paging_get_pd]
k_mem_paging_get_pd:
    mov eax, cr3
    ret

[global k_mem_paging_get_fault_addr]
k_mem_paging_get_fault_addr:
    mov eax, cr2
    ret