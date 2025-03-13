/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/* Fallback eager-paging PoD backend.
 *
 * This backend allocates and pages-in all areas eagerly on allocation using
 * pages provided by a `uk_alloc` page allocator. Paging out and writeback are
 * only ever done on demand.
 *
 * The PoD operations exposed follow the system PoD API detailed in
 * <uk/pod/default.h>; please consult that file for details.
 */

#ifndef __UK_POD_EAGER_H__
#define __UK_POD_EAGER_H__

#include <uk/alloc.h>
#include <uk/pod/types.h>

void *uk_pod_eager_alloc(struct uk_alloc *al, __sz npages, int locked,
			 const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_eager_free(struct uk_alloc *al, void *addr, __sz npages,
		      const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_eager_writeback(struct uk_alloc *al, void *addr, __sz npages,
			   const struct uk_pod_pgio *ops, void *arg, __sz base);

int uk_pod_eager_drop(struct uk_alloc *al, void *addr, __sz npages,
		      const struct uk_pod_pgio *ops, void *arg, __sz base);

/* Eager allocated pages are always in RAM */
static inline
int uk_pod_eager_lock(struct uk_alloc *al __unused,
		      void *addr __unused, __sz npages __unused,
		      const struct uk_pod_pgio *ops __unused,
		      void *arg __unused, __sz base __unused)
{
	return 0;
}

static inline
int uk_pod_eager_unlock(struct uk_alloc *al __unused,
			void *addr __unused, __sz npages __unused,
			const struct uk_pod_pgio *ops __unused,
			void *arg __unused, __sz base __unused)
{
	return 0;
}

/* Pages are eagerly populated on alloc; no-op */
static inline
int uk_pod_eager_populate(struct uk_alloc *al __unused,
			  void *addr __unused, __sz npages __unused,
			  const struct uk_pod_pgio *ops __unused,
			  void *arg __unused, __sz base __unused)
{
	return 0;
}

#endif /* __UK_POD_EAGER_H__ */
