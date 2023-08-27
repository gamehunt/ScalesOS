extern __k_int_isr_dispatcher

%macro isr_noerr 1 
    [global _isr%1]
    align 4
    _isr%1:
        push 0
        push %1
        jmp isr_stub
%endmacro

%macro isr_err 1 
    [global _isr%1]
    align 4
    _isr%1:
        push %1
        jmp isr_stub
%endmacro

isr_noerr 0
isr_noerr 1
isr_noerr 2
isr_noerr 3
isr_noerr 4
isr_noerr 5
isr_noerr 6
isr_noerr 7
isr_err   8
isr_err   9
isr_err   10
isr_err   11
isr_err   12
isr_err   13
isr_err   14
isr_noerr 15
isr_noerr 16
isr_err   17
isr_noerr 18
isr_noerr 19
isr_noerr 20
isr_err   21
isr_noerr 22
isr_noerr 23
isr_noerr 24
isr_noerr 25
isr_noerr 26
isr_noerr 27
isr_noerr 28
isr_err   29
isr_err   30
isr_noerr 31

%macro fix_gs 0
    push eax
    mov ax, 0x38
    mov gs, ax
    pop eax
%endmacro

isr_stub:
    fix_gs
    pushad
    cld
    mov edi, esp
    push edi
    call __k_int_isr_dispatcher
    mov esp, eax
    popad
    add esp, 8
    iret
