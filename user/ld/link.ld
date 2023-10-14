ENTRY(_start)

SECTIONS
{
	. = 0x40000000;

	.text BLOCK(4K) : ALIGN(4K)
	{
		code = .;
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	.eh_frame BLOCK(4K) : ALIGN(4K) {
		*(.eh_frame)
	}

	end = .;
}
