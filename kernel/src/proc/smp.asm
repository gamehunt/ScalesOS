bits 16
org 0

[global __k_proc_smp_ap_bootstrap]
__k_proc_smp_ap_bootstrap:
    cli

    lgdt cs:__k_proc_smp_ap_init_gdt_ptr

    mov eax, 0x00000010 
    mov cr4, eax ; 4mib pages

    mov edx, __k_proc_smp_ap_init_page
    mov cr3, edx ; put cr3

    mov eax, 0x80000001
    mov cr0, eax ; enable paging

    jmp 0x08:__k_proc_smp_ap_stub

[global __k_proc_smp_ap_init_gdt_ptr] 
align 16
__k_proc_smp_ap_init_gdt_ptr: 
    dw __k_proc_smp_ap_init_gdt_end-__k_proc_smp_ap_init_gdt-1
    dq __k_proc_smp_ap_init_gdt

[global __k_proc_smp_ap_init_gdt] 
align 16
__k_proc_smp_ap_init_gdt: 
    dq 0, 0
    dq 0x0000FFFF, 0x00CF9A00
    dq 0x0000FFFF, 0x008F9200
    dq 0x00000068, 0x00CF8900
__k_proc_smp_ap_init_gdt_end: 

[global __k_proc_smp_ap_init_page] 
align 4096
__k_proc_smp_ap_init_page: 
dd 0x00000083
times 1023 dd 0

bits 32
[global __k_proc_smp_ap_stub]
align 16
__k_proc_smp_ap_stub:
    mov eax, 0xDEADBEED
    hlt