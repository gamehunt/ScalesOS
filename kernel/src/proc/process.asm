[global __k_proc_process_save]
__k_proc_process_save:
    mov eax, [esp + 4] ; Get arg
    mov ecx, [esp]     ; Get EIP

    mov [eax + 8], ecx ; Save EIP
    mov [eax + 4], ebp ; Save EBP
    mov [eax + 0], esp ; Save ESP

    xor eax, eax       ; return 0
    ret

[global __k_proc_process_load]
__k_proc_process_load:
    mov ebx, [esp + 4] ; Get arg

    mov edx, [ebx + 12] ; Load new page directory if we need it
    mov eax, cr3

    cmp edx, eax
    je  finalize
    mov cr3, edx

finalize:
    mov ebp, [ebx + 4] ; Restore EBP
    mov esp, [ebx + 0] ; Restore ESP

    mov eax, 1    ; Make save function return 1
    jmp [ebx + 8] ; Jump