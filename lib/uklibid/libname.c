/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/libid.h>
#include <uk/arch/lcpu.h>

extern const char *namemap[];

const char *uk_libname(__u16 libid)
{
	if (unlikely(libid >= uk_libid_count()))
		return __NULL;

	return namemap[libid];
}
