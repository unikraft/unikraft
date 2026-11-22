/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKVMEM_COW_H__
#define __UKVMEM_COW_H__

#include <stdbool.h>
#include <uk/arch/types.h>
#include <uk/paging.h>

struct uk_alloc;
struct uk_falloc;
struct vmem_cow_page;

/*
 * One reference per mapping, including mappings awaiting TLB invalidation.
 * The containing object must outlive retirement. Access to an individual
 * reference, cloning and an exclusive writer must be serialized by its owner.
 */
struct vmem_cow_ref {
	struct vmem_cow_page *page;
	struct uk_paging_deferred_call retire;
	void (*released)(void *arg);
	void *arg;
	bool queued;
};

/*
 * References must initially be zeroed. Allocator instances must outlive all
 * references and support the caller's fault context without demand faults.
 * New frames are uninitialized. Failure leaves the destination empty and does
 * not call released; that callback only follows a successful initialization.
 */
int vmem_cow_ref_alloc(struct vmem_cow_ref *ref, struct uk_alloc *a,
		       struct uk_falloc *fa,
		       void (*released)(void *), void *arg);
/*
 * Adopt one aligned frame allocated by fa, preserving its contents. The frame
 * must be unpublished and exclusively owned by the caller; this does not add
 * ownership of an active mapping or a shared file-cache page. Success transfers
 * ownership to ref. Failure leaves ownership with the caller and ref empty.
 */
int vmem_cow_ref_adopt(struct vmem_cow_ref *ref, struct uk_alloc *a,
		       struct uk_falloc *fa, __paddr_t paddr,
		       void (*released)(void *), void *arg);
void vmem_cow_ref_clone(struct vmem_cow_ref *dst,
			const struct vmem_cow_ref *src,
			void (*released)(void *), void *arg);
/* The caller prevents writes to the source throughout the copy. */
int vmem_cow_ref_copy(struct vmem_cow_ref *dst,
		      const struct vmem_cow_ref *src, struct uk_pagetable *pt,
		      void (*released)(void *), void *arg);
__paddr_t vmem_cow_ref_paddr(const struct vmem_cow_ref *ref);
bool vmem_cow_ref_exclusive(const struct vmem_cow_ref *ref);

/*
 * Retire each mapping in its own invalidation batch, even if other references
 * remain. A NULL batch is only valid for an unpublished mapping or after its
 * TLB invalidation has already completed. The callback must not block or yield.
 */
void vmem_cow_ref_release(struct vmem_cow_ref *ref,
			  struct uk_paging_free_batch *batch);

#endif /* __UKVMEM_COW_H__ */
