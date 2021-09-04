 SECTIONS
{
	.my_section : {
     PROVIDE(my_section_start = .);
     KEEP (*(.my_section_entry1))
     KEEP (*(.my_section_entry2))
     KEEP (*(.my_section_entry3))
     KEEP (*(.my_section_entry4))
     PROVIDE(my_section_end = .);
	}
}
INSERT AFTER .text;

