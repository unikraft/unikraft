/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/vmem/process.h>

struct process_vas {
	struct uk_vas vas;
	struct uk_pagetable pt;
};

static void process_vas_release(struct uk_vas *vas)
{
	struct process_vas *p = __containerof(vas, struct process_vas, vas);
	struct uk_alloc *resident = vas->a;
	int rc;

	uk_vas_destroy(vas);
	/* VMA teardown released application frames. The remaining leaves
	 * are aliases of kernel-owned memory, including the direct map.
	 */
	rc = uk_paging_pt_free(&p->pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	if (rc)
		UK_CRASH("Cannot release process page tables: %d\n", rc);
	uk_free(resident, p);
}

static int process_vas_new(struct uk_alloc *resident, struct uk_vas **out)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct process_vas *p;
	__vaddr_t addr;
	unsigned int level;
	__pte_t pte;
	int rc;

	if (!kernel || kernel->range_start ||
	    kernel->range_end != UK_VAS_PROCESS_BASE)
		return -ENOTSUP;
	p = uk_malloc(resident, sizeof(*p));
	if (!p)
		return -ENOMEM;
	memset_isr(p, 0, sizeof(*p));
	rc = uk_paging_pt_clone_shared(&p->pt, kernel->pt, 0,
				       UK_VAS_PROCESS_BASE);
	if (rc)
		goto free_object;
	/* Inspect the snapshot, not the kernel before cloning: the paging
	 * clone holds the source PT lock while copying its root entries.
	 */
	for (addr = UK_VAS_PROCESS_BASE; addr < UK_VAS_PROCESS_END;
	     addr += UK_VAS_PROCESS_BASE) {
		level = UK_PAGING_PT_LEVELS - 1;
		rc = uk_paging_pt_walk(&p->pt, addr, &level, NULL, &pte);
		if (rc)
			goto free_pt;
		if (UK_PAGING_PT_Lx_PTE_PRESENT(pte, level)) {
			rc = -ENOTSUP;
			goto free_pt;
		}
	}
	rc = uk_vas_init_owned(&p->vas, &p->pt, resident,
			       process_vas_release);
	if (rc)
		goto free_pt;
	rc = uk_vas_set_bounds(&p->vas, UK_VAS_PROCESS_BASE,
			       UK_VAS_PROCESS_END);
	if (rc)
		goto free_pt;
	*out = &p->vas;
	return 0;

free_pt:
	/* No application VMA or frame is owned on this path. */
	uk_paging_pt_free(&p->pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
free_object:
	uk_free(resident, p);
	return rc;
}

int uk_vas_create_process(struct uk_alloc *resident, struct uk_vas **out)
{
	struct uk_vas *vas;
	int rc;

	if (!out)
		return -EINVAL;
	*out = NULL;
	if (!resident || !resident->malloc || !resident->free ||
	    !resident->palloc || !resident->pfree)
		return -EINVAL;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	rc = process_vas_new(resident, &vas);
	if (rc)
		return rc;
	rc = uk_vas_enable_cow(vas, resident);
	if (rc) {
		uk_vas_put(vas);
		return rc;
	}
	*out = vas;
	return 0;
}

int uk_vas_clone_process(struct uk_vas *src, struct uk_vas **out)
{
	struct uk_vas *vas;
	int rc;

	if (!out)
		return -EINVAL;
	*out = NULL;
	if (!src || src->release != process_vas_release ||
	    src->range_start != UK_VAS_PROCESS_BASE ||
	    src->range_end != UK_VAS_PROCESS_END)
		return -ENOTSUP;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	rc = process_vas_new(src->a, &vas);
	if (rc)
		return rc;
	rc = uk_vas_clone_cow(vas, src);
	if (rc) {
		uk_vas_put(vas);
		return rc;
	}
	*out = vas;
	return 0;
}
