SECTIONS
{
	. = ALIGN(1);
	.uk_libinfo :
	{
		KEEP (*(.uk_libinfo))
	}
}
INSERT AFTER .rodata;
