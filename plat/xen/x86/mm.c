/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: mm.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos
 *
 *        Date: Aug 2003, chages Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: memory management related functions
 *              contains buddy page allocator from Xen.
 *
 ****************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <uk/plat/common/sections.h>
#include <errno.h>
#include <uk/alloc.h>
#include <uk/plat/config.h>
#include <common/hypervisor.h>
#include <xen-x86/mm.h>
#include <xen/memory.h>
#include <uk/print.h>
#include <uk/assert.h>

#ifdef CONFIG_PARAVIRT
#include <xen-x86/mm_pv.h>
unsigned long *phys_to_machine_mapping;
#endif
unsigned long mfn_zero;
pgentry_t *pt_base;

/*
 * Make pt_pfn a new 'level' page table frame and hook it into the page
 * table at offset in previous level MFN (pref_l_mfn). pt_pfn is a guest
 * PFN.
 */
static pgentry_t pt_prot[PAGETABLE_LEVELS] = {
    L1_PROT,
    L2_PROT,
    L3_PROT,
#if defined(__x86_64__)
    L4_PROT,
#endif
};

static void new_pt_frame(unsigned long *pt_pfn, unsigned long prev_l_mfn,
                         unsigned long offset, unsigned long level)
{
    pgentry_t *tab;
    unsigned long pt_page = (unsigned long)pfn_to_virt(*pt_pfn);
#ifdef CONFIG_PARAVIRT
    mmu_update_t mmu_updates[1];
    int rc;
#endif

    uk_pr_debug("Allocating new L%lu pt frame for pfn=%lx, "
		"prev_l_mfn=%lx, offset=%lx\n",
		level, *pt_pfn, prev_l_mfn, offset);

    /* We need to clear the page, otherwise we might fail to map it
       as a page table page */
    memset((void*) pt_page, 0, PAGE_SIZE);

    UK_ASSERT(level >= 1 && level <= PAGETABLE_LEVELS);

#ifdef CONFIG_PARAVIRT
    /* Make PFN a page table page */
    tab = pt_base;
#if defined(__x86_64__)
    tab = pte_to_virt(tab[l4_table_offset(pt_page)]);
#endif
    tab = pte_to_virt(tab[l3_table_offset(pt_page)]);

    mmu_updates[0].ptr = (tab[l2_table_offset(pt_page)] & PAGE_MASK) +
        sizeof(pgentry_t) * l1_table_offset(pt_page);
    mmu_updates[0].val = (pgentry_t)pfn_to_mfn(*pt_pfn) << PAGE_SHIFT |
        (pt_prot[level - 1] & ~_PAGE_RW);

    if ( (rc = HYPERVISOR_mmu_update(mmu_updates, 1, NULL, DOMID_SELF)) < 0 )
	    UK_CRASH("PTE for new page table page could not be updated: "
		     "mmu_update failed with rc=%d\n", rc);

    /* Hook the new page table page into the hierarchy */
    mmu_updates[0].ptr =
        ((pgentry_t)prev_l_mfn << PAGE_SHIFT) + sizeof(pgentry_t) * offset;
    mmu_updates[0].val = (pgentry_t)pfn_to_mfn(*pt_pfn) << PAGE_SHIFT |
        pt_prot[level];

    if ( (rc = HYPERVISOR_mmu_update(mmu_updates, 1, NULL, DOMID_SELF)) < 0 )
	    UK_CRASH("mmu_update failed with rc=%d\n", rc);
#else
    tab = mfn_to_virt(prev_l_mfn);
    tab[offset] = (*pt_pfn << PAGE_SHIFT) | pt_prot[level];
#endif
    *pt_pfn += 1;
}

/*
 * Build the initial pagetable.
 */
void _init_mem_build_pagetable(unsigned long *start_pfn, unsigned long *max_pfn)
{
    unsigned long start_address, end_address;
    unsigned long pfn_to_map, pt_pfn = *start_pfn;
    pgentry_t *tab = pt_base, page;
    unsigned long pt_mfn = pfn_to_mfn(virt_to_pfn(pt_base));
    unsigned long offset;
#ifdef CONFIG_PARAVIRT
    static mmu_update_t mmu_updates[L1_PAGETABLE_ENTRIES + 1];
    int count = 0;
    int rc;
#endif

    /* Be conservative: even if we know there will be more pages already
       mapped, start the loop at the very beginning. */
    pfn_to_map = *start_pfn;

#ifdef CONFIG_PARAVIRT
    if ( *max_pfn >= virt_to_pfn(HYPERVISOR_VIRT_START) )
    {
	    uk_pr_warn("Trying to use Xen virtual space. "
		       "Truncating memory from %luMB to ",
		       ((unsigned long)pfn_to_virt(*max_pfn) - __TEXT)>>20);
	    *max_pfn = virt_to_pfn(HYPERVISOR_VIRT_START - PAGE_SIZE);
	    uk_pr_warn("%luMB\n",
		       ((unsigned long)pfn_to_virt(*max_pfn) - __TEXT)>>20);
    }
#else
    /* Round up to next 2MB boundary as we are using 2MB pages on HVMlite. */
    pfn_to_map = (pfn_to_map + L1_PAGETABLE_ENTRIES - 1) &
                 ~(L1_PAGETABLE_ENTRIES - 1);
#endif

    start_address = (unsigned long)pfn_to_virt(pfn_to_map);
    end_address = (unsigned long)pfn_to_virt(*max_pfn);

    /* We worked out the virtual memory range to map, now mapping loop */
    uk_pr_info("Mapping memory range 0x%lx - 0x%lx\n",
	       start_address, end_address);

    while ( start_address < end_address )
    {
        tab = pt_base;
        pt_mfn = pfn_to_mfn(virt_to_pfn(pt_base));

#if defined(__x86_64__)
        offset = l4_table_offset(start_address);
        /* Need new L3 pt frame */
        if ( !(tab[offset] & _PAGE_PRESENT) )
            new_pt_frame(&pt_pfn, pt_mfn, offset, L3_FRAME);

        page = tab[offset];
        pt_mfn = pte_to_mfn(page);
        tab = to_virt(mfn_to_pfn(pt_mfn) << PAGE_SHIFT);
#endif
        offset = l3_table_offset(start_address);
        /* Need new L2 pt frame */
        if ( !(tab[offset] & _PAGE_PRESENT) )
            new_pt_frame(&pt_pfn, pt_mfn, offset, L2_FRAME);

        page = tab[offset];
        pt_mfn = pte_to_mfn(page);
        tab = to_virt(mfn_to_pfn(pt_mfn) << PAGE_SHIFT);
        offset = l2_table_offset(start_address);
#ifdef CONFIG_PARAVIRT
        /* Need new L1 pt frame */
        if ( !(tab[offset] & _PAGE_PRESENT) )
            new_pt_frame(&pt_pfn, pt_mfn, offset, L1_FRAME);

        page = tab[offset];
        pt_mfn = pte_to_mfn(page);
        tab = to_virt(mfn_to_pfn(pt_mfn) << PAGE_SHIFT);
        offset = l1_table_offset(start_address);

        if ( !(tab[offset] & _PAGE_PRESENT) )
        {
            mmu_updates[count].ptr =
                ((pgentry_t)pt_mfn << PAGE_SHIFT) + sizeof(pgentry_t) * offset;
            mmu_updates[count].val =
                (pgentry_t)pfn_to_mfn(pfn_to_map) << PAGE_SHIFT | L1_PROT;
            count++;
        }
        pfn_to_map++;
        if ( count == L1_PAGETABLE_ENTRIES ||
             (count && pfn_to_map == *max_pfn) )
        {
            rc = HYPERVISOR_mmu_update(mmu_updates, count, NULL, DOMID_SELF);
            if ( rc < 0 )
		    UK_CRASH("PTE could not be updated. "
			     "mmu_update failed with rc=%d\n", rc);
            count = 0;
        }
        start_address += PAGE_SIZE;
#else
        if ( !(tab[offset] & _PAGE_PRESENT) )
            tab[offset] = (pgentry_t)pfn_to_map << PAGE_SHIFT |
                          L2_PROT | _PAGE_PSE;
        start_address += 1UL << L2_PAGETABLE_SHIFT;
#endif
    }

    *start_pfn = pt_pfn;
}

/*
 * Get the PTE for virtual address va if it exists. Otherwise NULL.
 */
static pgentry_t *get_pte(unsigned long va)
{
	unsigned long mfn;
	pgentry_t *tab;
	unsigned int offset;

	tab = pt_base;

#if defined(__x86_64__)
	offset = l4_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT))
		return NULL;

	mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(mfn);
#endif
	offset = l3_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT))
		return NULL;

	mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(mfn);
	offset = l2_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT))
		return NULL;

	if (tab[offset] & _PAGE_PSE)
		return &tab[offset];

	mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(mfn);
	offset = l1_table_offset(va);

	return &tab[offset];
}

/*
 * Return a valid PTE for a given virtual address.
 * If PTE does not exist, allocate page-table pages.
 */
static pgentry_t *need_pte(unsigned long va, struct uk_alloc *a)
{
	unsigned long pt_mfn;
	pgentry_t *tab;
	unsigned long pt_pfn;
	unsigned int offset;

	tab = pt_base;
	pt_mfn = virt_to_mfn(pt_base);
#if defined(__x86_64__)
	offset = l4_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT)) {
		pt_pfn = virt_to_pfn(uk_palloc(a, 1));
		if (!pt_pfn)
			return NULL;
		new_pt_frame(&pt_pfn, pt_mfn, offset, L3_FRAME);
	}
	UK_ASSERT(tab[offset] & _PAGE_PRESENT);

	pt_mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(pt_mfn);
#endif
	offset = l3_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT)) {
		pt_pfn = virt_to_pfn(uk_palloc(a, 1));
		if (!pt_pfn)
			return NULL;
		new_pt_frame(&pt_pfn, pt_mfn, offset, L2_FRAME);
	}
	UK_ASSERT(tab[offset] & _PAGE_PRESENT);

	pt_mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(pt_mfn);
	offset = l2_table_offset(va);
	if (!(tab[offset] & _PAGE_PRESENT)) {
		pt_pfn = virt_to_pfn(uk_palloc(a, 1));
		if (!pt_pfn)
			return NULL;
		new_pt_frame(&pt_pfn, pt_mfn, offset, L1_FRAME);
	}
	UK_ASSERT(tab[offset] & _PAGE_PRESENT);

	if (tab[offset] & _PAGE_PSE)
		return &tab[offset];

	pt_mfn = pte_to_mfn(tab[offset]);
	tab = mfn_to_virt(pt_mfn);
	offset = l1_table_offset(va);

	return &tab[offset];
}

/**
 * do_map_frames - Map an array of MFNs contiguously into virtual
 * address space starting at va.
 * @va: Starting address of the virtual address range
 * @mfns: Array of MFNs
 * @n: Number of MFNs
 * @stride: Stride used for selecting the MFNs on each iteration
 * @incr: Increment added to MFNs on each iteration
 * @id: Domain id
 * @err: Array of errors statuses
 * @prot: Page protection flags
 * @a: Memory allocator used when new page table entries are needed
 *
 * Note that either @stride or @incr must be non-zero, not both of
 * them. One should use a non-zero value for @stride when providing
 * an array of MFNs. @incr parameter should be used when only the
 * first MFN is provided and the subsequent MFNs values are simply
 * derived by adding @incr.
 */
#define MAP_BATCH ((STACK_SIZE / 4) / sizeof(mmu_update_t))
int do_map_frames(unsigned long va,
		const unsigned long *mfns, unsigned long n,
		unsigned long stride, unsigned long incr,
		domid_t id, int *err, unsigned long prot,
		struct uk_alloc *a)
{
	pgentry_t *pte = NULL;
	unsigned long mapped = 0;

	if (!mfns) {
		uk_pr_warn("do_map_frames: no mfns supplied\n");
		return -EINVAL;
	}

	uk_pr_debug("Mapping va=%p n=%lu, mfns[0]=0x%lx stride=%lu incr=%lu prot=0x%lx\n",
		    (void *) va, n, mfns[0], stride, incr, prot);

	if (err)
		memset(err, 0, n * sizeof(int));

	while (mapped < n) {
#ifdef CONFIG_PARAVIRT
		unsigned long i;
		int rc;
		unsigned long batched;

		if (err)
			batched = 1;
		else
			batched = n - mapped;

		if (batched > MAP_BATCH)
			batched = MAP_BATCH;

		mmu_update_t mmu_updates[batched];

		for (i = 0; i < batched; i++, va += PAGE_SIZE, pte++) {
			if (!pte || !(va & L1_MASK))
				pte = need_pte(va, a);
			if (!pte)
				return -ENOMEM;

			mmu_updates[i].ptr =
				virt_to_mach(pte) | MMU_NORMAL_PT_UPDATE;
			mmu_updates[i].val =
				((pgentry_t) (mfns[(mapped + i) * stride]
				+ (mapped + i) * incr) << PAGE_SHIFT) | prot;
		}

		rc = HYPERVISOR_mmu_update(mmu_updates, batched, NULL, id);
		if (rc < 0) {
			if (err)
				err[mapped * stride] = rc;
			else
				UK_CRASH(
					"Map %ld (%lx, ...) at %lx failed: %d.\n",
					batched,
					mfns[mapped * stride] + mapped * incr,
					va, rc);
		}
		mapped += batched;
#else
		if (!pte || !(va & L1_MASK))
			pte = need_pte(va & ~L1_MASK, a);
		if (!pte)
			return -ENOMEM;

		UK_ASSERT(!(*pte & _PAGE_PSE));
		pte[l1_table_offset(va)] =
			(pgentry_t) (((mfns[mapped * stride]
			+ mapped * incr) << PAGE_SHIFT) | prot);
		mapped++;
#endif
	}

	return 0;
}

static unsigned long demand_map_area_start;
static unsigned long demand_map_area_end;

unsigned long allocate_ondemand(unsigned long n, unsigned long align)
{
	unsigned long page_idx, contig = 0;

	/* Find a properly aligned run of n contiguous frames */
	for (page_idx = 0;
		page_idx <= DEMAND_MAP_PAGES - n;
		page_idx = (page_idx + contig + 1 + align - 1) & ~(align - 1)) {

		unsigned long addr =
			demand_map_area_start + page_idx * PAGE_SIZE;
		pgentry_t *pte = get_pte(addr);

		for (contig = 0; contig < n; contig++, addr += PAGE_SIZE) {
			if (!(addr & L1_MASK))
				pte = get_pte(addr);

			if (pte) {
				if (*pte & _PAGE_PRESENT)
					break;

				pte++;
			}
		}

		if (contig == n)
			break;
	}

	if (contig != n) {
		uk_pr_err("Failed to find %ld frames!\n", n);
		return 0;
	}

	return demand_map_area_start + page_idx * PAGE_SIZE;
}

/**
 * map_frames_ex - Map an array of MFNs contiguously into virtual
 * address space. Virtual addresses are allocated from the on demand
 * area.
 * @mfns: Array of MFNs
 * @n: Number of MFNs
 * @stride: Stride used for selecting the MFNs on each iteration
 * (e.g. 1 for every element, 0 always first element)
 * @incr: Increment added to MFNs on each iteration
 * @alignment: Alignment
 * @id: Domain id
 * @err: Array of errors statuses
 * @prot: Page protection flags
 * @a: Memory allocator used when new page table entries are needed
 *
 * Note that either @stride or @incr must be non-zero, not both of
 * them. One should use a non-zero value for @stride when providing
 * an array of MFNs. @incr parameter should be used when only the
 * first MFN is provided and the subsequent MFNs values are simply
 * derived by adding @incr.
 */
void *map_frames_ex(const unsigned long *mfns, unsigned long n,
		unsigned long stride, unsigned long incr,
		unsigned long alignment,
		domid_t id, int *err, unsigned long prot,
		struct uk_alloc *a)
{
	unsigned long va = allocate_ondemand(n, alignment);

	if (!va)
		return NULL;

	if (do_map_frames(va, mfns, n, stride, incr, id, err, prot, a))
		return NULL;

	return (void *) va;
}

/*
 * Unmap num_frames frames mapped at virtual address va.
 */
#define UNMAP_BATCH ((STACK_SIZE / 4) / sizeof(multicall_entry_t))
int unmap_frames(unsigned long va, unsigned long num_frames)
{
#ifdef CONFIG_PARAVIRT
	unsigned long i, n = UNMAP_BATCH;
	multicall_entry_t call[n];
	int ret;
#else
	pgentry_t *pte;
#endif

	UK_ASSERT(!((unsigned long) va & ~PAGE_MASK));

	uk_pr_debug("Unmapping va=%p, num=%lu\n",
		    (void *) va, num_frames);

	while (num_frames) {
#ifdef CONFIG_PARAVIRT
		if (n > num_frames)
			n = num_frames;

		for (i = 0; i < n; i++) {
			int arg = 0;
			/*
			 * simply update the PTE for the VA and
			 * invalidate TLB
			 */
			call[i].op = __HYPERVISOR_update_va_mapping;
			call[i].args[arg++] = va;
			call[i].args[arg++] = 0;
#ifdef __i386__
			call[i].args[arg++] = 0;
#endif
			call[i].args[arg++] = UVMF_INVLPG;

			va += PAGE_SIZE;
		}

		ret = HYPERVISOR_multicall(call, n);
		if (ret) {
			uk_pr_err("update_va_mapping hypercall failed with rc=%d.\n",
				  ret);
			return -ret;
		}

		for (i = 0; i < n; i++) {
			if (call[i].result) {
				uk_pr_err("update_va_mapping failed for with rc=%d.\n",
					  ret);
				return -(call[i].result);
			}
		}
		num_frames -= n;
#else
		pte = get_pte(va);
		if (pte) {
			UK_ASSERT(!(*pte & _PAGE_PSE));
			*pte = 0;
			invlpg(va);
		}
		va += PAGE_SIZE;
		num_frames--;
#endif
	}
	return 0;
}

/*
 * Mark portion of the address space read only.
 */
extern struct shared_info _libxenplat_shared_info;
void _init_mem_set_readonly(void *text, void *etext)
{
    unsigned long start_address =
        ((unsigned long) text + PAGE_SIZE - 1) & PAGE_MASK;
    unsigned long end_address = (unsigned long) etext;
    pgentry_t *tab = pt_base, page;
    unsigned long mfn;
    unsigned long offset;
    unsigned long page_size = PAGE_SIZE;
#ifdef CONFIG_PARAVIRT
    static mmu_update_t mmu_updates[L1_PAGETABLE_ENTRIES + 1];
    int count = 0;
    int rc;
#endif

    uk_pr_debug("Set %p-%p readonly\n", text, etext);
    mfn = pfn_to_mfn(virt_to_pfn(pt_base));

    while ( start_address + page_size <= end_address )
    {
        tab = pt_base;
        mfn = pfn_to_mfn(virt_to_pfn(pt_base));

#if defined(__x86_64__)
        offset = l4_table_offset(start_address);
        page = tab[offset];
        mfn = pte_to_mfn(page);
        tab = to_virt(mfn_to_pfn(mfn) << PAGE_SHIFT);
#endif
        offset = l3_table_offset(start_address);
        page = tab[offset];
        mfn = pte_to_mfn(page);
        tab = to_virt(mfn_to_pfn(mfn) << PAGE_SHIFT);
        offset = l2_table_offset(start_address);        
        if ( !(tab[offset] & _PAGE_PSE) )
        {
            page = tab[offset];
            mfn = pte_to_mfn(page);
            tab = to_virt(mfn_to_pfn(mfn) << PAGE_SHIFT);

            offset = l1_table_offset(start_address);
        }

        if ( start_address != (unsigned long)&_libxenplat_shared_info )
        {
#ifdef CONFIG_PARAVIRT
            mmu_updates[count].ptr = 
                ((pgentry_t)mfn << PAGE_SHIFT) + sizeof(pgentry_t) * offset;
            mmu_updates[count].val = tab[offset] & ~_PAGE_RW;
            count++;
#else
            tab[offset] &= ~_PAGE_RW;
#endif
        } else {
            uk_pr_debug("skipped %lx\n", start_address);
	}

        start_address += page_size;

#ifdef CONFIG_PARAVIRT
        if ( count == L1_PAGETABLE_ENTRIES || 
             start_address + page_size > end_address )
        {
            rc = HYPERVISOR_mmu_update(mmu_updates, count, NULL, DOMID_SELF);
            if ( rc < 0 )
		    UK_CRASH("PTE could not be updated\n");
            count = 0;
        }
#else
        if ( start_address == (1UL << L2_PAGETABLE_SHIFT) )
            page_size = 1UL << L2_PAGETABLE_SHIFT;
#endif
    }

#ifdef CONFIG_PARAVIRT
    {
        mmuext_op_t op = {
            .cmd = MMUEXT_TLB_FLUSH_ALL,
        };
        int count;
        HYPERVISOR_mmuext_op(&op, 1, &count, DOMID_SELF);
    }
#else
    write_cr3((unsigned long)pt_base);
#endif
}

/*
 * Clear some of the bootstrap memory
 */
void _init_mem_clear_bootstrap(void)
{
#ifdef CONFIG_PARAVIRT
    pte_t nullpte = { };
    int rc;
#else
    pgentry_t *pgt;
#endif

	uk_pr_debug("Clear bootstrapping memory: %p\n", (void *)__TEXT);

    /* Use first page as the CoW zero page */
	memset((void *)__TEXT, 0, PAGE_SIZE);
	mfn_zero = virt_to_mfn(__TEXT);
#ifdef CONFIG_PARAVIRT
    if ( (rc = HYPERVISOR_update_va_mapping(0, nullpte, UVMF_INVLPG)) )
	    uk_pr_err("Unable to unmap NULL page. rc=%d\n", rc);
#else
	pgt = get_pgt(__TEXT);
    *pgt = 0;
	invlpg(__TEXT);
#endif
}

#ifdef CONFIG_XEN_PV_BUILD_P2M
static unsigned long max_pfn;
static unsigned long *l3_list;
static unsigned long *l2_list_pages[P2M_ENTRIES];

void _arch_init_p2m(struct uk_alloc *a)
{
	unsigned long pfn;
	unsigned long *l2_list = NULL;

	max_pfn = HYPERVISOR_start_info->nr_pages;

	if (((max_pfn - 1) >> L3_P2M_SHIFT) > 0)
		UK_CRASH("Error: Too many pfns.\n");

	l3_list = uk_palloc(a, 1);
	for (pfn = 0; pfn < max_pfn; pfn += P2M_ENTRIES) {
		if (!(pfn % (P2M_ENTRIES * P2M_ENTRIES))) {
			l2_list = uk_palloc(a, 1);
			l3_list[L3_P2M_IDX(pfn)] = virt_to_mfn(l2_list);
			l2_list_pages[L3_P2M_IDX(pfn)] = l2_list;
		}

		l2_list[L2_P2M_IDX(pfn)] =
			virt_to_mfn(phys_to_machine_mapping + pfn);
	}
	HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list_list =
		virt_to_mfn(l3_list);
	HYPERVISOR_shared_info->arch.max_pfn = max_pfn;
}

void _arch_rebuild_p2m(void)
{
	unsigned long pfn;
	unsigned long *l2_list = NULL;

	for (pfn = 0; pfn < max_pfn; pfn += P2M_ENTRIES) {
		if (!(pfn % (P2M_ENTRIES * P2M_ENTRIES))) {
			l2_list = l2_list_pages[L3_P2M_IDX(pfn)];
			l3_list[L3_P2M_IDX(pfn)] = virt_to_mfn(l2_list);
		}

		l2_list[L2_P2M_IDX(pfn)] =
				virt_to_mfn(phys_to_machine_mapping + pfn);
	}
	HYPERVISOR_shared_info->arch.pfn_to_mfn_frame_list_list =
			virt_to_mfn(l3_list);
	HYPERVISOR_shared_info->arch.max_pfn = max_pfn;
}

#ifdef CONFIG_MIGRATION
void arch_mm_pre_suspend(void)
{

}
void arch_mm_post_suspend(int canceled)
{
	if (!canceled)
		_arch_rebuild_p2m();
}
#endif
#endif /* CONFIG_XEN_PV_BUILD_P2M */

void arch_mm_init(struct uk_alloc *a)
{
#ifdef CONFIG_XEN_PV_BUILD_P2M
	_arch_init_p2m(a);
#endif
}

void _init_mem_prepare(unsigned long *start_pfn, unsigned long *max_pfn)
{
#ifdef CONFIG_PARAVIRT
    phys_to_machine_mapping = (unsigned long *)HYPERVISOR_start_info->mfn_list;
    pt_base = (pgentry_t *)HYPERVISOR_start_info->pt_base;
    *start_pfn = PFN_UP(to_phys(pt_base)) + HYPERVISOR_start_info->nr_pt_frames;
    *max_pfn = HYPERVISOR_start_info->nr_pages;
#else
#error "Please port (see Mini-OS's arch_mm_preinit())"
#endif
}

/*
 * Reserve an area of virtual address space for mappings
 */

void _init_mem_demand_area(unsigned long start, unsigned long page_num)
{
	demand_map_area_start = start;
	demand_map_area_end = demand_map_area_start + page_num * PAGE_SIZE;

	uk_pr_info("Demand map pfns at %lx-%lx.\n",
		   demand_map_area_start, demand_map_area_end);
}
