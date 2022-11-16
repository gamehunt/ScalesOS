MBALIGN   equ  1 << 0 
MEMINFO   equ  1 << 1
VIDINFO   equ  1 << 2
FLAGS     equ  MBALIGN | MEMINFO | VIDINFO
MAGIC     equ  0x1BADB002
VIDMODE   equ  0
VIDHEIGHT equ  1024
VIDWIDTH  equ  1280
VIDDEPTH  equ  32
CHECKSUM  equ -(MAGIC + FLAGS)
 
KERNEL_VIRTUAL_BASE equ 0xC0000000                   ; 3GB
KERNEL_PAGE_NUMBER  equ (KERNEL_VIRTUAL_BASE >> 22)  ; Page directory index of kernel's 4MB PTE.

section .multiboot.data
align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM
	times 6 dd 0 
	dd VIDMODE
	dd VIDWIDTH
	dd VIDHEIGHT
	dd VIDDEPTH

section .initial_stack, nobits
align 4
stack_bottom:
resb 16384 ; 16kb
stack_top:

section .data
align 4096
boot_page_directory:
    dd 0x00000083
    times (KERNEL_PAGE_NUMBER - 1) dd 0                 ; Pages before kernel space.
    ; This page directory entry defines a 4MB page containing the kernel.
    dd 0x00000083
    times (1024 - KERNEL_PAGE_NUMBER) dd 0  ; Pages after the kernel image.
 

section .multiboot.text
global _start:function (_start.end - _start)
_start:
    mov ecx, (boot_page_directory - KERNEL_VIRTUAL_BASE)
    mov cr3, ecx                                        ; Load Page Directory Base Register.

    mov ecx, cr4
    or ecx, 0x00000010                          ; Set PSE bit in CR4 to enable 4MB pages.
    mov cr4, ecx
 
    mov ecx, cr0
    or ecx, 0x80000000                          ; Set PG bit in CR0 to enable paging.
    mov cr0, ecx

    lea ecx, [higher_half_stub]
    jmp ecx
.end:

extern kernel_main

section .text
higher_half_stub:
    mov dword [boot_page_directory], 0
    invlpg [0]
 
    mov esp, stack_top          ; set up the stack
 
    add ebx, KERNEL_VIRTUAL_BASE

    push ebx
    push eax
 
    xor   ebp, ebp
    call  kernel_main                  ; call kernel proper

    cli
    hlt                          ; halt machine should kernel return


