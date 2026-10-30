/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdio.h>
#include <string.h>
#include <libfdt.h>
#include <xen-arm/os.h>
#include <xen-arm/mm.h>
#include <xen-arm/shm.h>
#include <uk/arch/types.h>
#include <uk/essentials.h>
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

/* Attributes of an shm page, write access is added by uk_shm_map() */
#define SHM_PT_ATTR_RO							\
	(UK_PAL_PAGE_ATTR_PROT_READ					\
	|UK_PLAT_NATIVE_PAGE_ATTR_TYPE_NORMAL_WB			\
	|UK_ARCH_ARM64_PTE_ATTR_SH(UK_ARCH_ARM64_PTE_ATTR_SH_IS)	\
	)

struct uk_shm_entry {
	char id[UK_SHM_ID_MAXLEN];
	__vaddr_t vaddr;
	__sz size;
};

/* Always use L3 page tables to allow page-granularity permission control */
static lpae_t large_pgtbl[Ln_ENTRIES] __align(PAGE_SIZE);
static lpae_t pgtbl[SHM_NR_L3_TABLES][Ln_ENTRIES] __align(PAGE_SIZE);

static struct uk_pt shm_pt;

static __vaddr_t shm_next_va = FIX_SHM_START;

static struct uk_shm_entry shm_entries[CONFIG_XEN_SHM_MAX_ENTRIES];
static int shm_count;

static struct uk_spinlock shm_lock = UK_SPINLOCK_INITIALIZER();

static inline struct uk_shm_entry *shm_get_entry_by_id(const char *id)
{
	for (int i = 0; i < shm_count; i++) {
		if (strncmp(shm_entries[i].id, id, UK_SHM_ID_MAXLEN) == 0) {
			return &shm_entries[i];
		}
	}

	return NULL;
}

static int shm_link(__vaddr_t va, unsigned int lvl, void *tbl)
{
	__vaddr_t tv = (__vaddr_t)tbl;

	int rc = uk_pt_table_create(tv, lvl - 1);
	if (unlikely(rc)) {
		return rc;
	}

	return uk_pt_table_link(&shm_pt, va, lvl, uk_pt_frame_paddr(tv),
				UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);
}

/* Puts the shm page tables into the hierarchy */
static int shm_pt_init(void)
{
	__vaddr_t vaddr = FIX_SHM_START;
	int rc = uk_pt_init_active(&shm_pt);
	if (unlikely(rc)) {
		return rc;
	}

	/* Link in large pages tables (2MiB) */
	rc = shm_link(vaddr, UK_PAL_PT_LEVELS - 1, large_pgtbl);
	if (unlikely(rc)) {
		return rc;
	}

	/* Link in pages tables (4KiB) */
	for (size_t i = 0; i < SHM_NR_L3_TABLES; i++) {
		rc = shm_link(vaddr, UK_PAL_PAGE_LARGE_LEVEL, pgtbl[i]);
		if (unlikely(rc)) {
			return rc;
		}

		vaddr += UK_PAL_PAGE_LARGE_SIZE;
	}

	return 0;
}

static __vaddr_t shm_map_region(__paddr_t paddr, __sz size)
{
	__vaddr_t vaddr = shm_next_va;
	int rc;

	if (size > FIX_SHM_TOP - shm_next_va) {
		uk_pr_err("%s: mapping exceeds available VA space\n",
			  __func__);
		return 0;
	}

	for (__sz i = 0; i < (size >> PAGE_SHIFT); i++) {
		rc = uk_pt_map(&shm_pt, vaddr+i*PAGE_SIZE, paddr+i*PAGE_SIZE,
			       UK_PAL_PAGE_LEVEL, SHM_PT_ATTR_RO,
			       UK_PT_TMPL_NONE, UK_PT_TMPL_LEVEL_NONE);
		if (unlikely(rc)) {
			uk_pr_err("%s: could not map pa=0x%lx: %d\n",
				  __func__, (unsigned long)paddr, rc);
			return 0;
		}
	}

	shm_next_va += (size & PAGE_MASK);
	return vaddr;
}

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
		if (fdt_get_address(dtb, node, 0, &paddr, &size)) {
			uk_pr_err("%s: Invalid address/size\n", __func__);
			continue;
		}

		if ((paddr & ~PAGE_MASK) || (size & ~PAGE_MASK) || !size) {
			uk_pr_err("%s: region not page-aligned pa=0x%lx sz=0x%lx, skipping\n",
				  __func__, (unsigned long)paddr,
				  (unsigned long)size);
			continue;
		}

		offset = fdt_getprop(dtb, node, "xen,offset", &plen);
		if (plen == sizeof(*offset) && offset && *offset != 0) {
			uk_pr_warn("%s: unsupported non-zero 'xen,offset', skpping\n",
				   __func__);
			continue;
		}

		/* NB: Xen's boot device tree receives propety xen,shm-id,
		 *     but passes xen,id to booting domains
		 */
		id = fdt_getprop(dtb, node, "xen,id", &plen);
		if (!id) {
			uk_pr_err("%s: missing xen,id, skipping\n", __func__);
			continue;
		}

		if (plen < 1 || plen > UK_SHM_ID_MAXLEN || id[plen - 1] != '\0') {
			uk_pr_err("%s: id is not a valid string, skipping\n",
				  __func__);
			continue;
		}

		if (shm_count >= CONFIG_XEN_SHM_MAX_ENTRIES) {
			uk_pr_err("%s: too many shm entries\n", __func__);
			return;
		}

		vaddr = shm_map_region(paddr, size);
		if (!vaddr) {
			uk_pr_err("%s: unable to map region %s, skipping\n",
				  __func__, id);
			continue;
		}

		shm_entries[shm_count].vaddr = vaddr;
		shm_entries[shm_count].size = size;
		strcpy(shm_entries[shm_count].id, id);
		shm_count++;

		uk_pr_debug("%s: shm id=%s pa=0x%lx sz=0x%lx va=0x%lx\n",
			    __func__, id, (unsigned long)paddr,
			    (unsigned long)size, vaddr);
	}
}

int uk_plat_xen_shm_get(const char *id, __sz len, bool readonly,
			__off offset, void **mem)
{
	struct uk_shm_entry *entry = __NULL;
	unsigned long flags;
	unsigned long page_attr;
	__vaddr_t va;
	int rc;

	if (!mem) {
		return -EINVAL;
	}
	*mem = NULL;

	if (!id) {
		uk_pr_err("%s: id is NULL\n", __func__);
		return -EINVAL;
	}

	if (len == 0) {
		uk_pr_err("%s: len is 0 for id='%s'\n", __func__, id);
		return -EINVAL;
	}

	if (((unsigned long)offset & ~PAGE_MASK) ||
	    (len & ~PAGE_MASK)) {
		uk_pr_err("%s: offset/len not page-aligned id='%s' off=0x%lx len=0x%lx\n",
			  __func__, id, (unsigned long)offset,
			  (unsigned long)len);
		return -EINVAL;
	}

	entry = shm_get_entry_by_id(id);
	if (!entry) {
		uk_pr_err("%s: id '%s' not found\n", __func__, id);
		return -ENOENT;
	}

	if (offset < 0 || len > entry->size || (__sz)offset > entry->size - len) {
		uk_pr_err("%s: bounds check failed id='%s' off=0x%lx len=0x%lx size=0x%lx\n",
			  __func__, id, (unsigned long)offset,
			  (unsigned long)len, (unsigned long)entry->size);
		return -EINVAL;
	}

	va = entry->vaddr + (__sz)offset;
	page_attr = SHM_PT_ATTR_RO;
	if (!readonly) {
		page_attr |= UK_PAL_PAGE_ATTR_PROT_WRITE;
	}

	uk_spin_lock_irqsave(&shm_lock, flags);

	for (__sz i = 0; i < (len >> PAGE_SHIFT); i++) {
		rc = uk_pt_set_attr(&shm_pt, va + i*PAGE_SIZE,
				    UK_PAL_PAGE_LEVEL, page_attr);
		if (unlikely(rc)) {
			uk_pr_err("%s: could not set permissions id='%s': %d\n",
				  __func__, id, rc);
			/* Mark the result as NULL */
			va = 0;
			break;
		}
	}

	uk_spin_unlock_irqrestore(&shm_lock, flags);

	*mem = (void *)va;
	return (va == 0) ? -EIO : 0;
}
