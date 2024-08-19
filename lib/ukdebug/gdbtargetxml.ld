SECTIONS
{
	/* Target description file for GDB stub */
	.gdb.target.xml :
	{
		PROVIDE(__gdb_target_xml_start = .);
		KEEP(*(.gdb.target.xml))
		PROVIDE(__gdb_target_xml_end = .);
	}
}
INSERT AFTER .rodata;
