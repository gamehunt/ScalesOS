[global k_dev_timer_read_tsc]
k_dev_timer_read_tsc:
    rdtsc
    ret

[global __k_dev_timer_compute_speed]
__k_dev_timer_compute_speed:
    push ebp
    mov  ebp, esp

    in   al, 0x61 
    and  al, 0xDD
    or   al, 0x1
    out  0x61, al

    mov  al, 0xb2
    out  0x43, al

    mov  al, 0x9b
    out  0x42, al
    in   al, 0x60

    mov  al, 0x2e
    out  0x42, al

    in   al, 0x61
    and  al, 0xde
    out  0x61, al

    or   al, 0x01
    out  0x61, al

    rdtsc

    mov esi, [ebp + 8]
    mov edi, [ebp + 12]

    mov [esi], edx
    mov [edi], eax

    push edx ; High
    push eax ; Low

w:
    in   al, 0x61
    and  al, 0x20
    jnz  w

    rdtsc
    
    pop ebx ; Low
    pop ecx ; High

    sub eax, ebx
    sbb edx, ecx

    pop ebp
    ret
