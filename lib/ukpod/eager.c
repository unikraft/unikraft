/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Fallback implementation of page-on-demand that immediately allocates and
 * populates returned pages. Paging out is done synchronously on release.
 */

#include <uk/errptr.h>
#include <uk/pod/eager.h>

/* We define this for convenience, without wishing to pull in a libc */
#ifndef ENOMEM
#define ENOMEM 12
#endif

static
__ssz loop_pgin(uk_pod_pgin_func func,
		char *addr, __sz npages, void *arg, __sz base)
{
	__sz done = 0;
	__ssz r;

	while (done < npages) {
		r = func(addr, npages - done, __PADDR_INV, arg, base + done);
		if (unlikely(r < 0))
			return r;
		UK_ASSERT(r > 0); /* func must make progress or error out */
		done += r;
		addr += r * PAGE_SIZE;
	}
	return done;
}

static
__ssz loop_pgwb(uk_pod_pgwb_func func,
		const char *addr, __sz npages, void *arg, __sz base)
{
	__sz done = 0;
	__ssz r;

	while (done < npages) {
		r = func(addr, npages - done, arg, base + done);
		if (unlikely(r < 0))
			return r;
		UK_ASSERT(r > 0); /* func must make progress or error out */
		done += r;
		addr += r * PAGE_SIZE;
	}
	return done;
}

void *uk_pod_eager_alloc(struct uk_alloc *al,
			 __sz npages, int locked __unused,
			 const struct uk_pod_pgio *ops,
			 void *arg, __sz base)
{
	__ssz r;
	void *buf = uk_palloc(al, npages);

	if (unlikely(!buf))
		return ERR2PTR(-ENOMEM);
	UK_ASSERT(PAGE_ALIGNED((__uptr)buf));
	r = loop_pgin(ops->pagein, buf, npages, arg, base);
	/* Loop has either completed or errored (r < 0 || r == npages) */
	if (unlikely(r < 0)) {
		uk_pfree(al, buf, npages);
		return ERR2PTR(r);
	}
	return buf;
}

int uk_pod_eager_free(struct uk_alloc *al,
		      void *addr, __sz npages,
		      const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	__ssz r = loop_pgwb(ops->writeback, addr, npages, arg, base);

	/* Loop has either completed or errored (r < 0 || r == npages) */
	if (unlikely(r < 0))
		return r;
	uk_pfree(al, addr, npages);
	return 0;
}

int uk_pod_eager_writeback(struct uk_alloc *al __unused,
			   void *addr, __sz npages,
			   const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	__ssz r = loop_pgwb(ops->writeback, addr, npages, arg, base);

	/* Loop has either completed or errored (r < 0 || r == npages) */
	if (unlikely(r < 0))
		return r;
	return 0;
}

/* Simulate drop by calling pagein again */
int uk_pod_eager_drop(struct uk_alloc *al __unused,
		      void *addr, __sz npages,
		      const struct uk_pod_pgio *ops, void *arg, __sz base)
{
	__ssz r = ops->pagein(addr, npages, __PADDR_INV, arg, base);

	/* Loop has either completed or errored (r < 0 || r == npages) */
	if (unlikely(r < 0))
		return r;
	return 0;
}
