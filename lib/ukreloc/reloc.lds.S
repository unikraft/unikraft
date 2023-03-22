/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h> /* for __PAGE_SIZE */

PHDRS
{
	dynamic PT_DYNAMIC;
}

SECTIONS
{
	. = ALIGN(__PAGE_SIZE);
	.ukreloc :
	{
		_ukreloc_start = .;
		. += 4;
		KEEP(*(.ukreloc))
	} :data

	/* For whatever reason, .dynsym wants to stay before .rela.dyn.
	 * This does not bother us, even if we strip it, since we are compiling
	 * statically, .dynsym will be insignificantly small.
	 */
	.dynsym :
	{
		*(.dynsym)
	} :data

	.rela.dyn :
	{
		*(.rela)
		*(.rela.*)
	} :data
	_ukreloc_end = .;
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
	} :dynamic :data

	.dynstr :
	{
		*(.dynstr)
	} :data
}
INSERT AFTER .comment
