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
#include <xen-x86/os.h>
#include <xen-x86/mm.h>
#include <xen/memory.h>
#include <uk/print.h>
#include <uk/assert.h>

#ifdef CONFIG_PARAVIRT
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

    uk_printd(DLVL_EXTRA, "Allocating new L%lu pt frame for pfn=%lx, "
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
	    uk_printd(DLVL_WARN, "Trying to use Xen virtual space. "
		      "Truncating memory from %luMB to ",
		      ((unsigned long)pfn_to_virt(*max_pfn) -
		       (unsigned long)&_text)>>20);
	    *max_pfn = virt_to_pfn(HYPERVISOR_VIRT_START - PAGE_SIZE);
	    uk_printd(DLVL_WARN, "%luMB\n",
		      ((unsigned long)pfn_to_virt(*max_pfn) - 
		       (unsigned long)&_text)>>20);
    }
#else
    /* Round up to next 2MB boundary as we are using 2MB pages on HVMlite. */
    pfn_to_map = (pfn_to_map + L1_PAGETABLE_ENTRIES - 1) &
                 ~(L1_PAGETABLE_ENTRIES - 1);
#endif

    start_address = (unsigned long)pfn_to_virt(pfn_to_map);
    end_address = (unsigned long)pfn_to_virt(*max_pfn);

    /* We worked out the virtual memory range to map, now mapping loop */
    uk_printd(DLVL_INFO, "Mapping memory range 0x%lx - 0x%lx\n",
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

    uk_printd(DLVL_EXTRA, "Set %p-%p readonly\n", text, etext);
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
            uk_printd(DLVL_EXTRA, "skipped %lx\n", start_address);
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

    uk_printd(DLVL_EXTRA, "Clear bootstrapping memory: %p\n", &_text);

    /* Use first page as the CoW zero page */
    memset(&_text, 0, PAGE_SIZE);
    mfn_zero = virt_to_mfn((unsigned long) &_text);
#ifdef CONFIG_PARAVIRT
    if ( (rc = HYPERVISOR_update_va_mapping(0, nullpte, UVMF_INVLPG)) )
	    uk_printd(DLVL_ERR, "Unable to unmap NULL page. rc=%d\n", rc);
#else
    pgt = get_pgt((unsigned long)&_text);
    *pgt = 0;
    invlpg((unsigned long)&_text);
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
