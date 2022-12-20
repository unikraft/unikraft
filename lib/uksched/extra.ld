SECTIONS
{
	.uk_thread_inittab : {
		PROVIDE(_uk_thread_inittab_start = .);
		KEEP (*(.uk_thread_inittab0))
		KEEP (*(.uk_thread_inittab0.*))
		KEEP (*(.uk_thread_inittab1))
		KEEP (*(.uk_thread_inittab1.*))
		KEEP (*(.uk_thread_inittab2))
		KEEP (*(.uk_thread_inittab2.*))
		KEEP (*(.uk_thread_inittab3))
		KEEP (*(.uk_thread_inittab3.*))
		KEEP (*(.uk_thread_inittab4))
		KEEP (*(.uk_thread_inittab4.*))
		KEEP (*(.uk_thread_inittab5))
		KEEP (*(.uk_thread_inittab5.*))
		KEEP (*(.uk_thread_inittab6))
		KEEP (*(.uk_thread_inittab6.*))
		KEEP (*(.uk_thread_inittab7))
		KEEP (*(.uk_thread_inittab7.*))
		KEEP (*(.uk_thread_inittab8))
		KEEP (*(.uk_thread_inittab8.*))
		KEEP (*(.uk_thread_inittab9))
		KEEP (*(.uk_thread_inittab9.*))
		PROVIDE(_uk_thread_inittab_end = .);
	}
}
INSERT AFTER .rodata;
