OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
	. = 0;
	.tcs : {
		*(.tcs*)
	}

	. = ALIGN(4096);
	.text : {
		*(.text*)
		*(.rodata*)
	}

	. = ALIGN(4096);
	.data : {
		*(.data*)
	}

	/DISCARD/ : {
		*(.data*)
		*(.comment*)
		*(.note*)
		*(.debug*)
		*(.eh_frame*)
	}
}
