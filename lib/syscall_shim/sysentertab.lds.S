/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/limits.h> /* for __PAGE_SIZE */
#include <uk/plat/common/common.lds.h> /* for UK_SEGMENT_DATA */

SECTIONS
{
	. = ALIGN(__PAGE_SIZE);

	.uk_sysentertab :
	{
		PROVIDE(uk_sysentertab_start = .);
		KEEP(*(SORT_BY_NAME(.uk_sysentertab[0-9])))
		PROVIDE(uk_sysentertab_end = .);
	} UK_SEGMENT_DATA
}
INSERT AFTER .rodata;
