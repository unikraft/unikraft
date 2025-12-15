/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/falloc.h>

int uk_falloc_get_total_memory(struct uk_falloc *fa, __sz *out)
{
	UK_ASSERT(fa);
	UK_ASSERT(out);

	*out = uk_load_n(&fa->total_memory);

	return 0;
}

int uk_falloc_get_free_memory(struct uk_falloc *fa, __sz *out)
{

	UK_ASSERT(fa);
	UK_ASSERT(out);

	*out = uk_load_n(&fa->free_memory);

	return 0;
}

int uk_falloc_add_total_memory(struct uk_falloc *fa, __sz add)
{
	UK_ASSERT(fa);

	uk_add_fetch(&fa->total_memory, add);

	return 0;
}

int uk_falloc_add_free_memory(struct uk_falloc *fa, __sz add)
{
	UK_ASSERT(fa);

	uk_add_fetch(&fa->free_memory, add);

	return 0;
}

int uk_falloc_sub_total_memory(struct uk_falloc *fa, __sz sub)
{
	UK_ASSERT(fa);

	uk_sub_fetch(&fa->total_memory, sub);

	return 0;
}

int uk_falloc_sub_free_memory(struct uk_falloc *fa, __sz sub)
{
	UK_ASSERT(fa);

	uk_sub_fetch(&fa->free_memory, sub);

	return 0;
}
