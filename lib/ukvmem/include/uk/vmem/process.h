/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __UK_VMEM_PROCESS_H__
#define __UK_VMEM_PROCESS_H__

#include <uk/config.h>
#include <uk/vmem.h>

#if CONFIG_LIBUKVMEM_PROCESS_VAS
/* Share the first top-level branch with the kernel. Reserve the remainder
 * of the lower canonical half for private application mappings. Derive the
 * split from the configured page-table geometry, including five-level x86.
 */
#define UK_VAS_PROCESS_BASE \
	UK_PAGING_PAGE_Lx_SIZE(UK_PAGING_PT_LEVELS - 1)
#define UK_VAS_PROCESS_END \
	(UK_VAS_PROCESS_BASE * \
	 (UK_PAGING_PT_Lx_PTES(UK_PAGING_PT_LEVELS - 1) / 2))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create an unpublished owned process VAS with one caller reference.
 * The kernel VAS must be bounded to [0, UK_VAS_PROCESS_BASE); its other
 * lower-half top-level branches must be empty. Fixed high kernel aliases
 * are retained. New kernel mappings must use the shared branch.
 *
 * The resident allocator must supply non-faulting, non-blocking malloc/free
 * and palloc/pfree operations and outlive this VAS and every clone. The
 * caller permits interrupts and runs on a shared kernel stack. The result
 * is NULL on failure. Release the returned reference with uk_vas_put().
 */
int uk_vas_create_process(struct uk_alloc *resident, struct uk_vas **out);

/**
 * Clone a VAS created by these functions, returning one unpublished owned
 * reference. The caller holds src live and follows uk_vas_clone_cow()'s
 * shared-stack and interrupt requirements. Only supported COW mappings
 * are accepted. The result is NULL on failure; src remains usable.
 */
int uk_vas_clone_process(struct uk_vas *src, struct uk_vas **out);

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_LIBUKVMEM_PROCESS_VAS */

#endif /* __UK_VMEM_PROCESS_H__ */
