SECTIONS
{
	.posix_process_clonetab : {
		PROVIDE(_posix_process_clonetab_start = .);
		KEEP (*(.posix_process_clonetab))
		KEEP (*(.posix_process_clonetab.*))
		PROVIDE(_posix_process_clonetab_end = .);
	}
}
INSERT AFTER .rodata;
