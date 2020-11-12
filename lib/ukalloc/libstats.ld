SECTIONS
{
	.uk_alloc_libstats : {
		KEEP (*(.uk_alloc_libstats))
	}
}
INSERT BEFORE .data;
