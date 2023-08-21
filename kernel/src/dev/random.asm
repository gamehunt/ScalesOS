extern __k_dev_random_get_value_fallback

[global __k_dev_random_get_value]
__k_dev_random_get_value:
	mov eax, 1
	cpuid
	test edx, 0x40000000 ; (1 << 30) RDRAND feature
	jz no_rdrand
	rdrand eax
	jmp end
no_rdrand:
	call __k_dev_random_get_value_fallback
end:
	ret
