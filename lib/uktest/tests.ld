SECTIONS
{
	.uk_testtab : {
		uk_testtab_start = .;
		KEEP(*(SORT_BY_NAME(.uk_testtab_*)))
		uk_testtab_end = .;
		uk_asserttab_start = .;
		KEEP(*(SORT_BY_NAME(.uk_asserttab_*)))
		uk_asserttab_end = .;
	}
}
INSERT AFTER .text;
