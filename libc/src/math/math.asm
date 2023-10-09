; Those are platform-specific
; TODO move it to arch/ or someth

[global __arch_math_sqrt]
__arch_math_sqrt:
	push ebp
	mov  ebp, esp
	fld qword [ebp + 8]
	fsqrt
	leave
	ret

[global __arch_math_atan2]
__arch_math_atan2:
	push ebp
	mov  ebp, esp
	fld qword [ebp + 8]
	fld qword [ebp + 12]
	fpatan
	leave
	ret

[global __arch_math_pow]
__arch_math_pow:
	push ebp
	mov  ebp, esp
	fld qword [ebp + 8]  ; base
	fld qword [ebp + 12] ; exponent
	fyl2x
	; TODO
	leave
	ret
