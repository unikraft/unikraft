/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/libid.h>
#include <uk/arch/lcpu.h>
#include <uk/assert.h>
#if CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC
#include <string.h>
#endif /* CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC */

extern const char *namemap[];

const char *uk_libname(__u16 libid)
{
	if (unlikely(libid >= uk_libid_count()))
		return __NULL;

	return namemap[libid];
}

#if CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC
__u16 uk_libid(const char *libname)
{
	__u16 libid;

	UK_ASSERT(libname);

	for (libid = 0; libid < uk_libid_count(); ++libid) {
		if (strcmp(libname, namemap[libid]) == 0)
			return libid;
	}

	/* no library with given name found */
	return UKLIBID_NONE;
}
#endif /* CONFIG_HAVE_LIBC || CONFIG_LIBNOLIBC */

__u16 uk_libid_count(void)
{
	return uk_libid_static_count();
}
