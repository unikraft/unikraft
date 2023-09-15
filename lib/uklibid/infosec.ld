SECTIONS
{
	. = ALIGN(1);
	.uk_libinfo :
	{
		PROVIDE(uk_libinfo_start = .);
		KEEP (*(.uk_libinfo))
		PROVIDE(uk_libinfo_end = .);
	}
}
INSERT AFTER .rodata;
