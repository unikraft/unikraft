SECTIONS
{
	. = ALIGN(0x8);
	.uk_fs_driver_list : {
		PROVIDE(uk_fs_driver_list_start = .);
		KEEP(*(SORT_BY_NAME(.uk_fs_driver_*)));
		PROVIDE(uk_fs_driver_list_end = .);
	}
}
INSERT AFTER .rodata;
