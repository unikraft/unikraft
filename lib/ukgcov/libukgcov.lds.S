SECTIONS {
	. = ALIGN(8);
	.gcov_info : {
		PROVIDE (__gcov_info_start = .);
		KEEP (*(.gcov_info))
		PROVIDE (__gcov_info_end = .);
	}
}
INSERT AFTER .rodata
