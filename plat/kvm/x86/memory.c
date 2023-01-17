/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/plat/common/memory.h>
#include <uk/plat/common/bootinfo.h>

int ukplat_memregion_count(void)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();

	UK_ASSERT(bi);

	return (int)bi->mrds.count;
}

int ukplat_memregion_get(int i, struct ukplat_memregion_desc **mrd)
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();

	UK_ASSERT(bi);
	UK_ASSERT(i >= 0);

	if (unlikely((__u32)i >= bi->mrds.count))
		return -1;

	*mrd = &bi->mrds.mrds[i];
	return 0;
}

int _ukplat_mem_mappings_init(void)
{
	return 0;
}
