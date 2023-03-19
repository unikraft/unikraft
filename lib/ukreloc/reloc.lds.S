/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h> /* for __PAGE_SIZE */
#include <uk/plat/common/common.lds.h> /* for UK_SEGMENT_DATA */

#define UKRELOC_SIGNATURE_SIZE		4

PHDRS
{
	dynamic PT_DYNAMIC;
}

SECTIONS
{
	. = ALIGN(__PAGE_SIZE);
	.uk_reloc :
	{
		_uk_reloc_start = .;
		/* Start gathering static uk_reloc from the signature onwards */
		. += UKRELOC_SIGNATURE_SIZE;
		KEEP(*(.uk_reloc))
	} UK_SEGMENT_DATA

	/* For whatever reason, .dynsym wants to stay before .rela.dyn.
	 * This does not bother us, even if we strip it, since we are compiling
	 * statically, .dynsym will be insignificantly small.
	 */
	.dynsym :
	{
		*(.dynsym)
	} UK_SEGMENT_DATA

	.rela.dyn :
	{
		*(.rela)
		*(.rela.*)
	} UK_SEGMENT_DATA
	_uk_reloc_end = .;
}
INSERT BEFORE .bss

/******************************************************************************/
/****************************** .bss section **********************************/
/******************************************************************************/

/******************************************************************************/
/**************************** .comment section ********************************/
/******************************************************************************/

SECTIONS
{
	.dynamic :
	{
		*(.dynamic)
	} :dynamic UK_SEGMENT_DATA

	.dynstr :
	{
		*(.dynstr)
	} UK_SEGMENT_DATA
}
INSERT AFTER .comment
