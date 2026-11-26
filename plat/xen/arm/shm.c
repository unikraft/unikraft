/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>
#include <libfdt.h>
#include <xen-arm/os.h>
#include <xen-arm/mm.h>
#include <xen-arm/shm.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
#include <uk/init.h>
#include <uk/ofw/fdt.h>
#include <uk/plat/pal/page.h>
#include <uk/print.h>
#include <uk/pt.h>
#include <uk/spinlock.h>

#define DT_SHM_COMPAT "xen,shared-memory-v1"

#define DT_GET_NEXT_SHM(dtb, node) \
	fdt_node_offset_by_compatible((dtb), (node), DT_SHM_COMPAT)

#define DT_GET_FIRST_SHM(dtb) \
	DT_GET_NEXT_SHM((dtb), -1)

#define DT_FOREACH_SHM(dtb, _item_name)				\
	for (int _item_name = DT_GET_FIRST_SHM(dtb);		\
	     _item_name >= 0;					\
	     _item_name = DT_GET_NEXT_SHM(dtb, _item_name))

#define SHM_PT_ATTR_INIT					\
		(UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB	\
		| UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS))

#define SHM_PT_ATTR_RO	\
	(SHM_PT_ATTR_INIT | UK_PAL_PAGE_ATTR_PROT_READ)

#define SHM_PT_ATTR_RW	\
	(SHM_PT_ATTR_RO | UK_PAL_PAGE_ATTR_PROT_WRITE)

struct uk_shm_entry {
	char id[UK_PLAT_XEN_SHM_ID_MAXLEN];
	__vaddr_t vaddr;
	__sz size;
	unsigned long attr;
};

/* Always use L3 page tables to allow page-granularity permission control */
static lpae_t large_pgtbl[Ln_ENTRIES] __align(PAGE_SIZE);
static lpae_t pgtbl[SHM_NR_L3_TABLES][Ln_ENTRIES] __align(PAGE_SIZE);

static struct uk_pt shm_pt;

static __vaddr_t shm_next_va = FIX_SHM_START;

static struct uk_shm_entry shm_entries[CONFIG_XEN_SHM_MAX_ENTRIES];
static int shm_count;

static struct uk_spinlock shm_lock = UK_SPINLOCK_INITIALIZER();

static __bool shm_is_sealed;

#if CONFIG_XEN_SHM_SEAL
static int shm_seal(struct uk_init_ctx *ctx __unused)
{
	shm_is_sealed = true;
	return 0;
}
uk_late_initcall_prio(shm_seal, 0, UK_PRIO_LATEST);
#endif /* CONFIG_XEN_SHM_SEAL */

static inline struct uk_shm_entry *shm_get_entry_by_id(const char *id)
{
	for (int i = 0; i < shm_count; i++) {
		if (!strncmp(shm_entries[i].id, id, UK_PLAT_XEN_SHM_ID_MAXLEN))
			return &shm_entries[i];
	}

	return NULL;
}

static int shm_link(__vaddr_t va, unsigned int lvl, void *tbl)
{
	__vaddr_t tv = (__vaddr_t)tbl;
	int rc = uk_pt_table_create(tv, lvl - 1);

	if (unlikely(rc))
		return rc;

	return uk_pt_table_link(&shm_pt, va, lvl, uk_pt_frame_paddr(tv),
				UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);
}

/* Put the shm page tables into the hierarchy */
static int shm_pt_init(void)
{
	__vaddr_t vaddr = FIX_SHM_START;
	int rc = uk_pt_init_active(&shm_pt);

	if (unlikely(rc))
		return rc;

	/* Link in large pages tables (2MiB) */
	rc = shm_link(vaddr, UK_PAL_PT_LEVELS - 1, large_pgtbl);
	if (unlikely(rc))
		return rc;

	/* Link in pages tables (4KiB) */
	for (size_t i = 0; i < SHM_NR_L3_TABLES; i++) {
		rc = shm_link(vaddr, UK_PAL_PAGE_LARGE_LEVEL, pgtbl[i]);
		if (unlikely(rc))
			return rc;

		vaddr += UK_PAL_PAGE_LARGE_SIZE;
	}

	return 0;
}

/**
 * Map a static shared memory region.
 *
 * @param paddr
 *   The physical address at which the region starts
 * @param size
 *   Size, in bytes, of the mapping
 * @param vaddr
 *   Set to the resulting virtual address on success (otherwise untouched)
 * @return
 *   0 on success
 *   -ENOMEM if there is not enough virtual memory space available
 *   -EFAULT if a page could not be mapped
 */
static int shm_map_region(__paddr_t paddr, __sz size, __vaddr_t *vaddr)
{
	int rc;
	__sz mapped = 0;

	if (size > FIX_SHM_TOP - shm_next_va) {
		uk_pr_err("%s: mapping exceeds available VA space\n",
			  __func__);
		return -ENOMEM;
	}

	for (__sz i = 0; i < (size >> PAGE_SHIFT); i++) {
		rc = uk_pt_map(&shm_pt,
			       shm_next_va + mapped, paddr + mapped,
			       UK_PAL_PAGE_LEVEL, SHM_PT_ATTR_INIT,
			       UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);

		if (unlikely(rc)) {
			uk_pr_err("%s: could not map pa=0x%lx: %d\n",
				  __func__, (unsigned long)paddr, rc);

			while (mapped) {
				mapped -= PAGE_SIZE;
				uk_pt_unmap(&shm_pt, shm_next_va + mapped,
					    UK_PAL_PAGE_LEVEL, __NULL);
			}

			return -EFAULT;
		}

		mapped += PAGE_SIZE;
	}

	*vaddr = shm_next_va;
	shm_next_va += mapped;

	return 0;
}

static inline __bool _is_shm_id_invalid(int plen, const char *id)
{
	return plen < 1 ||
		plen > UK_PLAT_XEN_SHM_ID_MAXLEN ||
		id[plen - 1] != '\0';
}

static inline __bool _is_shm_misaligned(__u64 paddr, __u64 size)
{
	return (paddr & ~PAGE_MASK) || (size & ~PAGE_MASK) || !size;
}

static inline __bool _is_nonzero_offset(int plen, const __u64 *offset)
{
	return plen == sizeof(*offset) && offset && fdt64_ld(offset);
}

#define WARN_SKIP(msg, ...) \
	uk_pr_warn("%s: " msg ", skipping\n", __func__, ##__VA_ARGS__)

void uk_plat_xen_shm_init(void)
{
	const void *dtb = HYPERVISOR_dtb;
	__u64 paddr, size;
	const __u64 *offset;
	__vaddr_t vaddr;
	const char *id;
	int plen;
	int rc;

	rc = shm_pt_init();
	if (unlikely(rc)) {
		uk_pr_err("%s: could not link page tables: %d\n",
			  __func__, rc);
		return;
	}

	DT_FOREACH_SHM(dtb, node) {
		rc = fdt_get_address(dtb, node, 0, &paddr, &size);
		if (unlikely(rc)) {
			WARN_SKIP("invalid address/size");
			continue;
		}

		if (unlikely(_is_shm_misaligned(paddr, size))) {
			WARN_SKIP("region not page-aligned pa=0x%lx sz=0x%lx",
				  (unsigned long)paddr, (unsigned long)size);
			continue;
		}

		offset = fdt_getprop(dtb, node, "xen,offset", &plen);
		if (unlikely(_is_nonzero_offset(plen, offset))) {
			WARN_SKIP("unsupported non-zero 'xen,offset'");
			continue;
		}

		/* NB: Xen's boot device tree receives property xen,shm-id,
		 *     but passes xen,id to booting domains
		 */
		id = fdt_getprop(dtb, node, "xen,id", &plen);
		if (unlikely(!id)) {
			WARN_SKIP("missing xen,id");
			continue;
		}

		if (unlikely(_is_shm_id_invalid(plen, id))) {
			WARN_SKIP("id is not a valid string");
			continue;
		}

		if (unlikely(shm_count >= CONFIG_XEN_SHM_MAX_ENTRIES)) {
			uk_pr_err("%s: too many shm entries\n", __func__);
			return;
		}

		rc = shm_map_region(paddr, size, &vaddr);
		if (unlikely(rc)) {
			WARN_SKIP("unable to map region %s: %d", id, rc);
			continue;
		}

		shm_entries[shm_count].vaddr = vaddr;
		shm_entries[shm_count].size = size;
		strncpy(shm_entries[shm_count].id, id,
			UK_PLAT_XEN_SHM_ID_MAXLEN - 1);
		shm_entries[shm_count].attr = SHM_PT_ATTR_INIT;
		shm_count++;

		uk_pr_debug("%s: shm id=%s pa=0x%lx sz=0x%lx va=0x%lx\n",
			    __func__, id, (unsigned long)paddr,
			    (unsigned long)size, vaddr);
	}
}

int uk_plat_xen_shm_configure(const char *id, __bool writeable)
{
	int rc = 0;
	unsigned long flags;
	unsigned long page_attr = writeable ? SHM_PT_ATTR_RW : SHM_PT_ATTR_RO;
	struct uk_shm_entry *ent;

	if (shm_is_sealed)
		return -EPERM;

	if (!id)
		return -EINVAL;

	ent = shm_get_entry_by_id(id);
	if (!ent)
		return -ENOENT;

	uk_spin_lock_irqsave(&shm_lock, flags);

	for (__sz i = 0; i < ent->size >> PAGE_SHIFT; i++) {
		rc = uk_pt_set_attr(&shm_pt, ent->vaddr + i * PAGE_SIZE,
				    UK_PAL_PAGE_LEVEL, page_attr);

		/* Rollback previous permission on pages already configured */
		if (unlikely(rc)) {
			while (i--) {
				uk_pt_set_attr(&shm_pt,
					       ent->vaddr + i * PAGE_SIZE,
					       UK_PAL_PAGE_LEVEL,
					       ent->attr);
			}

			rc = -EIO;
			break;
		}
	}

	uk_spin_unlock_irqrestore(&shm_lock, flags);

	if (!rc)
		ent->attr = page_attr;

	return rc;
}

int uk_plat_xen_shm_get(const char *id, void **mem, __sz *size)
{
	struct uk_shm_entry *ent = __NULL;

	if (!id)
		return -EINVAL;

	ent = shm_get_entry_by_id(id);
	if (!ent)
		return -ENOENT;

	if (ent->attr == SHM_PT_ATTR_INIT)
		return -EBADSLT;

	if (mem)
		*mem = (void *)ent->vaddr;

	if (size)
		*size = ent->size;

	return 0;
}
