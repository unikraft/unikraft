SECTIONS
{
   . = ALIGN(0x1000);
   uk_ctortab = .;
   .uk_ctortab :
   {
       KEEP(*(SORT_BY_NAME(.uk_ctortab[0-7])))
       LONG(0)
   }
}
INSERT AFTER .rodata
