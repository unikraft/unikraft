SECTIONS
{
	. = ALIGN(0x8);
	.posix_netlink_protocol_list : {
		PROVIDE(posix_netlink_protocol_list_start = .);
		KEEP(*(SORT_BY_NAME(.posix_netlink_protocol_*)));
		PROVIDE(posix_netlink_protocol_list_end = .);
	}
}
INSERT AFTER .rodata;
