/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKVMEM_COW_ALLOC_H__
#define __UKVMEM_COW_ALLOC_H__

#include <uk/arch/types.h>

struct uk_alloc;

struct vmem_cow_alloc_stats {
	__sz objects;
	__sz slabs;
};

/*
 * Internal fixed-size metadata allocator. The parent supplies resident,
 * page-aligned palloc/pfree operations that cannot fault, block or yield.
 * Objects have 16-byte alignment. One control page lives until destroy;
 * completely empty object slabs are returned immediately to the parent.
 * This interface is not registered in the global allocator list.
 */
struct uk_alloc *vmem_cow_alloc_create(struct uk_alloc *parent, __sz obj_size);
void vmem_cow_alloc_stats(struct uk_alloc *a, struct vmem_cow_alloc_stats *out);

/* Caller quiesces allocator operations; -EBUSY means objects remain. */
int vmem_cow_alloc_destroy(struct uk_alloc *a);

#endif /* __UKVMEM_COW_ALLOC_H__ */
