SECTIONS
{
    .got            : { *(.got) *(.igot) }
    .got.plt        : { *(.got.plt) *(.igot.plt) }
}
INSERT BEFORE .data
