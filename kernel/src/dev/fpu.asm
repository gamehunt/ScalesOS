section .data
testword: 
    dw 0xFFFF

section .text
[global __k_dev_fpu_probe]
__k_dev_fpu_probe:
    mov edx, cr0
    and edx, 0xffffffeb ; clear EM and ET bits
    mov cr0, edx
    fninit
    fnstsw   [testword]
    cmp word [testword], 0
    jne no
    mov eax, 1
    jmp end
no:
    mov eax, 0
end:
    ret

[global __k_dev_fpu_control_word]
__k_dev_fpu_control_word:
    fldcw [esp + 4]
    ret