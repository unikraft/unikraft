SECTIONS
{
	.uk_alloc_libstats : {
		PROVIDE(_uk_alloc_libstats_start = .);
		KEEP (*(.uk_alloc_libstats))
		PROVIDE(_uk_alloc_libstats_end = .);
	}
}
INSERT AFTER .data;
