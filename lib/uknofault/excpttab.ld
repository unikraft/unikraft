SECTIONS
{
	.uk_excpttab :
	{
		PROVIDE(uk_excpttab_start = .);
		KEEP(*(.uk_excpttab))
		PROVIDE(uk_excpttab_end = .);
	}
}
INSERT AFTER .rodata;
