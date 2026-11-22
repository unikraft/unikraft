/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKVMEM_COW_VMA_H__
#define __UKVMEM_COW_VMA_H__

#include <uk/arch/types.h>

struct uk_vas;
struct uk_vma;
struct uk_alloc;
struct uk_vma_shared_ref;

/* The VMA list and every retirement batch have already been drained. */
void vmem_cow_vas_destroy(struct uk_vas *vas);

/* Copy backend state; the caller initializes the generic VMA fields. */
int vmem_shared_anon_clone(struct uk_vma *vma, struct uk_alloc *a,
			   struct uk_vma **out);

/* The caller holds the VAS lock and validated the range and backend. */
void vmem_shared_anon_ref(struct uk_vma *vma, __vaddr_t addr,
			  struct uk_vma_shared_ref *out);

#endif /* __UKVMEM_COW_VMA_H__ */
