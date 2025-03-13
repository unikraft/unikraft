/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Page I/O ops for anonymous memory */

#include <uk/pod/anon.h>

/**
 * Page-in function for anonymous memory; fills pages with zeros.
 */
__ssz uk_pod_anon_pagein(void *addr, __sz npages, __paddr_t pa __unused,
			 void *arg __unused, __sz pgoff __unused)
{
	/* We implement page clearing ourselves for 2 reasons:
	 * - remove dependency on memset (and thus a libc)
	 * - faster code because we know the buf & size are page-aligned
	 */
	char *__align(PAGE_SIZE) p;

	UK_ASSERT(PAGE_ALIGNED((__uptr)addr));
	p = addr;
	/* The compiler will unroll this byte-wise loop with appropriate SIMD */
	for (__sz i = 0; i < npages * PAGE_SIZE; i++)
		p[i] = 0;
	return npages;
}

/**
 * Writeback function for anonymous memory; no-op.
 */
__ssz uk_pod_anon_writeback(const void *addr __unused, __sz npages,
			    void *arg __unused, __sz pgoff __unused)
{
	return npages;
}

/**
 * Pageout function for anonymous memory; no-op.
 */
void uk_pod_anon_pageout(const void *addr __unused, __sz npages __unused,
			 __paddr_t pa __unused, void *arg __unused,
			 __sz pgoff __unused)
{
}

/**
 * Page I/O ops for anonymous memory.
 */
const struct uk_pod_pgio uk_pod_anon_ops = {
	.pagein = uk_pod_anon_pagein,
	.writeback = uk_pod_anon_writeback,
	.pageout = uk_pod_anon_pageout
};
