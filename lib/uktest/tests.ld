SECTIONS
{
	.uk_testsuite_list : {
		PROVIDE(uk_testsuite = .);
		KEEP (*(.uk_testsuite_list))
		LONG(0x0)
	}
}
INSERT AFTER .text;
