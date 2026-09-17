/* SPDX-License-Identifier: BSD-3-Clause */
#include <stdint.h>
#include <string.h>
#include <xen-arm/shm.h>
#include <xen-arm/mm.h>
#include <uk/print.h>
#include <uk/spinlock.h>

static struct uk_spinlock shm_lock = UK_SPINLOCK_INITIALIZER();

static inline void tlb_flush_va(unsigned long vaddr)
{
	__u64 page_number = vaddr >> PAGE_SHIFT;

	__asm__ __volatile__("dsb ishst\n"
			     "tlbi vaae1is, %x0\n"
			     "dsb ish\n"
			     "isb\n"
			     :: "r" (page_number) : "memory");
}

void *uk_shm_map(const char *id, __sz len, __u16 perm, __off offset)
{
	struct uk_shm_entry *entry = __NULL;
	unsigned long flags;
	unsigned long addr, end;
	void *result;
	int l3_idx;
	lpae_t *pte;
	lpae_t val;
	int i;

	if (!id) {
		uk_pr_err("%s: id is NULL\n", __func__);
		return __NULL;
	}

	if (len == 0) {
		uk_pr_err("%s: len is 0 for id='%s'\n", __func__, id);
		return __NULL;
	}

	if (((unsigned long)offset & ~PAGE_MASK) ||
	    (len & ~PAGE_MASK)) {
		uk_pr_err("%s: offset/len not page-aligned id='%s' off=0x%lx len=0x%lx\n",
			  __func__, id, (unsigned long)offset,
			  (unsigned long)len);
		return __NULL;
	}

	uk_spin_lock_irqsave(&shm_lock, flags);

	for (i = 0; i < shm_count; i++) {
		if (strncmp(shm_entries[i].id, id, UK_SHM_ID_MAXLEN) == 0) {
			entry = &shm_entries[i];
			break;
		}
	}
	if (!entry) {
		uk_pr_err("%s: id '%s' not found\n", __func__, id);
		uk_spin_unlock_irqrestore(&shm_lock, flags);
		return __NULL;
	}

	if ((__sz)offset + len > entry->size) {
		uk_pr_err("%s: bounds check failed id='%s' off=0x%lx len=0x%lx size=0x%lx\n",
			  __func__, id, (unsigned long)offset,
			  (unsigned long)len, (unsigned long)entry->size);
		uk_spin_unlock_irqrestore(&shm_lock, flags);
		return __NULL;
	}

	/* Set PTE permissions for the requested range */
	addr = entry->vaddr + (__sz)offset;
	end = addr + len;

	while (addr < end) {
		l3_idx = (addr - FIX_SHM_START) >> L2_SHIFT;
		pte = &shm_l3_pgtable[l3_idx][l3_pgt_idx(addr)];
		val = *pte;

		/*
		 * Per-call permission: each call unconditionally sets
		 * the requested access. (e.g. RW → RO → RW supported)
		 */
		if (perm & UK_SHM_RO)
			val |= ATTR_AP(ATTR_AP_RO);   /* set RO bit */
		else
			val &= ~ATTR_AP(ATTR_AP_RO);  /* clear RO bit */

		set_pgt_entry(pte, val);
		tlb_flush_va(addr);
		addr += PAGE_SIZE;
	}

	result = (void *)(entry->vaddr + (__sz)offset);

	uk_spin_unlock_irqrestore(&shm_lock, flags);

	return result;
}
