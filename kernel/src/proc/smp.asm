bits 16 ; base = 0x1000

[global __k_proc_smp_ap_trampoline]
__k_proc_smp_ap_trampoline: ; 0x0
    cli
    cld
    jmp 0x0:0x1040
    align 16

__k_proc_smp_ap_init_gdt:   ; 0x10
    dd 0, 0
    dd 0x0000FFFF, 0x00CF9A00
    dd 0x0000FFFF, 0x00CF9200
    dd 0x00000068, 0x00CF8900

__k_proc_smp_ap_init_gdt_ptr: ; 0x30
    dw __k_proc_smp_ap_init_gdt_ptr-__k_proc_smp_ap_init_gdt-1
    dd 0x1010 ; __k_proc_smp_ap_init_gdt 
    dd 0, 0 ; pad
    align 64

__k_proc_smp_ap_bootstrap: ; 0x40
    xor ax, ax
    mov ds, ax

    lgdt [0x1030] ; __k_proc_smp_ap_init_gdt_ptr 

    mov eax, cr0
    or  eax, 0x1
    mov cr0, eax ; enter protected mode 
    
    jmp 0x8:0x1060 ; __k_proc_smp_ap_pm_bootstrap
    align 32

bits 32

extern __k_proc_smp_stack
extern __k_proc_smp_ap_startup
extern initial_page_directory

__k_proc_smp_ap_pm_bootstrap: ; 0x60
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    mov eax, [initial_page_directory - 0xC0000000] 
    mov cr3, eax ; load page directory

    mov eax, cr4
    or  eax, 0x00000010 ; Set PSE bit in CR4 to enable 4MB pages.
    mov cr4, eax

    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax ; enable paging

    mov eax, [__k_proc_smp_stack]
    mov esp, eax ; load stack

    mov  eax, __k_proc_smp_ap_startup
    call eax ; jump to C routine
    
    cli ; Should be never reached 
    hlt
    
