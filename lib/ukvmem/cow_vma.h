/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UKVMEM_COW_VMA_H__
#define __UKVMEM_COW_VMA_H__

struct uk_vas;
struct uk_vma;
struct uk_alloc;

/* The VMA list and every retirement batch have already been drained. */
void vmem_cow_vas_destroy(struct uk_vas *vas);

/* Copy backend state; the caller initializes the generic VMA fields. */
int vmem_shared_anon_clone(struct uk_vma *vma, struct uk_alloc *a,
			   struct uk_vma **out);

#endif /* __UKVMEM_COW_VMA_H__ */
