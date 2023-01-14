SECTIONS
{
	.uk_posix_clonetab : {
		PROVIDE(_uk_posix_clonetab_start = .);
		KEEP (*(.uk_posix_clonetab0))
		KEEP (*(.uk_posix_clonetab0.*))
		KEEP (*(.uk_posix_clonetab1))
		KEEP (*(.uk_posix_clonetab1.*))
		KEEP (*(.uk_posix_clonetab2))
		KEEP (*(.uk_posix_clonetab2.*))
		KEEP (*(.uk_posix_clonetab3))
		KEEP (*(.uk_posix_clonetab3.*))
		KEEP (*(.uk_posix_clonetab4))
		KEEP (*(.uk_posix_clonetab4.*))
		KEEP (*(.uk_posix_clonetab5))
		KEEP (*(.uk_posix_clonetab5.*))
		KEEP (*(.uk_posix_clonetab6))
		KEEP (*(.uk_posix_clonetab6.*))
		KEEP (*(.uk_posix_clonetab7))
		KEEP (*(.uk_posix_clonetab7.*))
		KEEP (*(.uk_posix_clonetab8))
		KEEP (*(.uk_posix_clonetab8.*))
		KEEP (*(.uk_posix_clonetab9))
		KEEP (*(.uk_posix_clonetab9.*))
		PROVIDE(_uk_posix_clonetab_end = .);
	}
}
INSERT AFTER .text;
