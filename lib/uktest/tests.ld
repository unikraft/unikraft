SECTIONS
{
	.uk_testtab ALIGN(8) : {
		uk_testtab_start = .;
		KEEP(*(SORT_BY_NAME(.uk_testtab_*)))
		uk_testtab_end = .;
		uk_asserttab_start = .;
		KEEP(*(.uk_asserttab))
		uk_asserttab_end = .;
	}
}
INSERT AFTER .data;
