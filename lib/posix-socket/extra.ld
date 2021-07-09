SECTIONS
{
	.posix_socket_driver_list : {
		PROVIDE(posix_socket_driver_list_start = .);
		KEEP (*(.posix_socket_driver_list))
    PROVIDE(posix_socket_driver_list_end = .);
	}
}
INSERT AFTER .text;
