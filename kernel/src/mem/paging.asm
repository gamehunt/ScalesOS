[global k_mem_paging_get_pd]
k_mem_paging_get_pd:
    mov eax, cr3
    ret