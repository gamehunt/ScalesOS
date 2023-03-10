[GLOBAL k_mem_load_gdt]
k_mem_load_gdt:
	mov	eax, [esp+4]	; Get the pointer to the GDT, passed as a parameter.
	lgdt	 [eax]		; Load the new GDT pointer.

	mov	ax, 0x10	    ; 0x10 is the offset in the GDT to our data segment
	mov	ds, ax		    ; Load all data segment selectors
	mov	es, ax
	mov	fs, ax
	mov	ss, ax
	
    mov ax, 0x38
    mov gs, ax

    jmp	0x08:.flush	    ; 0x08 is the offset to our code segment: Far jump!
.flush:
	ret

[global __k_mem_gdt_flush_tss]
__k_mem_gdt_flush_tss:
    mov ax, 0x28
	ltr ax
	ret
