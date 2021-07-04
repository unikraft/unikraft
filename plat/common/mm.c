/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Stefan Teodorescu <stefanl.teodorescu@gmail.com>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <uk/assert.h>
#include <uk/list.h>
#include <uk/print.h>
#include <uk/plat/mm.h>
#include <uk/plat/common/mem_layout.h>
#include <uk/framealloc.h>
#include <uk/framealloc/bbuddy.h>

typedef int (*pte_write_func_t)(__vaddr_t, size_t, __pte_t, size_t);

/* TODO: there is currently just one page table used, Update accordingly when
 * using multiple page tables,
 */
struct uk_pagetable ukplat_default_pt;

struct uk_pagetable *ukplat_get_active_pt(void)
{
	return &ukplat_default_pt;
}

static void _refresh_pt_pages_cache(struct uk_pagetable *pt)
{
	size_t needed_pages = PT_PAGES_CACHE_MAX_PAGES - pt->pages_cache_length;
	size_t i;
	__vaddr_t page;
	struct uk_pagetable *active_pt = ukplat_get_active_pt();

	page = pt->fa->palloc(pt->fa, needed_pages);
	ukplat_page_map_many(active_pt, page + pt->virt_offset,
			page, needed_pages,
			PAGE_PROT_READ | PAGE_PROT_WRITE, 0);
	if (pt != active_pt) {
		ukplat_page_map_many(pt, page + pt->virt_offset, page,
				needed_pages, PAGE_PROT_READ | PAGE_PROT_WRITE,
				0);
	}

	for (i = 0; i < needed_pages; i++)
		pt->pages_cache[pt->pages_cache_length++] = page + + pt->virt_offset + i * PAGE_SIZE;
}

/**
 * Allocate a page table for a given level (in the PT hierarchy).
 *
 * @param level: the level of the needed page table.
 *
 * @return: virtual address of newly allocated page table or PAGE_INVALID
 * on failure.
 */
static __pte_t uk_pt_alloc_table(struct uk_pagetable *pt, size_t level, int is_initmem)
{
	__vaddr_t pt_vaddr;
	int rc;
	unsigned long plat_flags;

	plat_flags = ukplat_mm_supported_features();

	if (pt->pages_cache_length < PT_PAGES_CACHE_MIN_PAGES)
		_refresh_pt_pages_cache(pt);
	pt_vaddr = pt->pages_cache[--pt->pages_cache_length];

	UK_ASSERT(PAGE_PRESENT(ukplat_virt_to_pte(pt, pt_vaddr)));

	if (plat_flags & UKPLAT_READONLY_PAGETABLE) {
		rc = ukplat_page_set_prot(pt, pt_vaddr,
				PAGE_PROT_READ | PAGE_PROT_WRITE);
		if (rc)
			return PAGE_INVALID;
	}

	memset((void *) pt_vaddr, 0,
		sizeof(__pte_t) * pagetable_entries[level - 1]);

	/*
	 * When using this function on Xen for the initmem part, the page
	 * must not be set to read-only, as we are currently writing
	 * directly into it. All page tables will be set later to read-only
	 * before setting the new pt_base.
	 */
	if (!is_initmem && (plat_flags & UKPLAT_READONLY_PAGETABLE)) {
		rc = ukplat_page_set_prot(pt, pt_vaddr, PAGE_PROT_READ);
		if (rc)
			return PAGE_INVALID;
	}

	/*
	 * This is an L(n + 1) entry, so we set L(n + 1) flags
	 * (Index in pagetable_protections is level of PT - 1)
	 */
	return pt_virt_to_mframe(pt, pt_vaddr) | pagetable_protections[level];
}

static int uk_pt_release_if_unused(struct uk_pagetable *pt, __vaddr_t vaddr,
		__vaddr_t pt_vaddr, __vaddr_t parent_pt_vaddr, size_t level)
{
	size_t i;
	int rc;

	if (!PAGE_ALIGNED(pt_vaddr) || !PAGE_ALIGNED(parent_pt_vaddr)) {
		uk_pr_err("Table's address must be aligned to page size\n");
		return -1;
	}

	for (i = 0; i < pagetable_entries[level - 1]; i++) {
		if (PAGE_PRESENT(ukarch_pte_read(pt_vaddr, i, level)))
			return 0;
	}

	rc = ukarch_pte_write(parent_pt_vaddr, Lx_OFFSET(vaddr, level + 1), 0,
		level + 1);
	if (rc)
		return -1;

	ukarch_flush_tlb_entry(parent_pt_vaddr);

	/* TODO: check if this always works */
	pt->pages_cache[++pt->pages_cache_length] = pt_vaddr;

	return 0;
}

static int _page_map(struct uk_pagetable *pt, __vaddr_t vaddr, __paddr_t paddr,
	  unsigned long prot, unsigned long flags, int is_initmem,
	  pte_write_func_t pte_write)
{
	__pte_t pte;
	__vaddr_t pt_vaddr = pt->pt_base;
	int rc;

	if (!PAGE_ALIGNED(vaddr)) {
		uk_pr_err("Virt address must be aligned to page size\n");
		return -1;
	}
	if (flags & PAGE_FLAG_LARGE && !PAGE_LARGE_ALIGNED(vaddr)) {
		uk_pr_err("Virt ddress must be aligned to large page size\n");
		return -1;
	}

	if ((flags & PAGE_FLAG_LARGE) &&
			!(ukplat_mm_supported_features()
				& UKPLAT_SUPPORT_LARGE_PAGES)) {
		uk_pr_err("Large pages are not supported this platform\n");
		return -1;
	}

	if (paddr == PAGE_PADDR_ANY) {
		paddr = uk_get_next_free_frame(pt->fa, flags);
		if (paddr == PAGE_INVALID)
			return -1;
	} else if (!PAGE_ALIGNED(paddr)) {
		uk_pr_err("Phys address must be aligned to page size\n");
		return -1;
	} else if ((flags & PAGE_FLAG_LARGE) && !PAGE_LARGE_ALIGNED(paddr)) {
		uk_pr_err("Phys address must be aligned to large page size\n");
		return -1;
	}

	/*
	 * XXX: On 64-bits architectures (x86_64 and arm64) the hierarchical
	 * page tables have a 4 level layout. This implementation will need a
	 * revision when introducing support for 32-bits architectures, since
	 * there are only 3 levels of page tables.
	 */
	pte = ukarch_pte_read(pt_vaddr, L4_OFFSET(vaddr), 4);
	if (!PAGE_PRESENT(pte)) {
		pte = uk_pt_alloc_table(pt, 3, is_initmem);
		if (pte == PAGE_INVALID)
			return -1;

		rc = pte_write(pt_vaddr, L4_OFFSET(vaddr), pte, 4);
		if (rc)
			return -1;
	}

	pt_vaddr = pt_pte_to_virt(pt, pte);
	pte = ukarch_pte_read(pt_vaddr, L3_OFFSET(vaddr), 3);
	if (!PAGE_PRESENT(pte)) {
		pte = uk_pt_alloc_table(pt, 2, is_initmem);
		if (pte == PAGE_INVALID)
			return -1;

		rc = pte_write(pt_vaddr, L3_OFFSET(vaddr), pte, 3);
		if (rc)
			return -1;
	}

	pt_vaddr = pt_pte_to_virt(pt, pte);
	pte = ukarch_pte_read(pt_vaddr, L2_OFFSET(vaddr), 2);
	if (flags & PAGE_FLAG_LARGE) {
		if (PAGE_PRESENT(pte)) {
			uk_pr_err("Virtual address %p is within a range already mapped by a large page\n",
					(void *) vaddr);
			return -1;
		}

		pte = ukarch_pte_create(pte_to_mframe(paddr), prot, 2);
		rc = pte_write(pt_vaddr, L2_OFFSET(vaddr), pte, 2);
		if (rc)
			return -1;

		return 0;
	}
	if (!PAGE_PRESENT(pte)) {
		pte = uk_pt_alloc_table(pt, 1, is_initmem);
		if (pte == PAGE_INVALID)
			return -1;

		rc = pte_write(pt_vaddr, L2_OFFSET(vaddr), pte, 2);
		if (rc)
			return -1;
	}

	pt_vaddr = pt_pte_to_virt(pt, pte);
	pte = ukarch_pte_read(pt_vaddr, L1_OFFSET(vaddr), 1);
	if (!PAGE_PRESENT(pte) || (flags & PAGE_FLAG_SHARED)) {
		pte = ukarch_pte_create(paddr, prot, 1);
		rc = pte_write(pt_vaddr, L1_OFFSET(vaddr), pte, 1);
		if (rc)
			return -1;
	} else {
		uk_pr_info("Virtual address 0x%08lx is already mapped\n",
			vaddr);
		return -1;
	}

	return 0;
}

int _initmem_page_map(struct uk_pagetable *pt, __vaddr_t vaddr, __paddr_t paddr,
		unsigned long prot, unsigned long flags)
{
	return _page_map(pt, vaddr, paddr, prot, flags, 1,
		_ukarch_pte_write_raw);
}


int ukplat_page_map(struct uk_pagetable *pt,__vaddr_t vaddr, __paddr_t paddr,
		unsigned long prot, unsigned long flags)
{
	return _page_map(pt, vaddr, paddr, prot, flags, 0, ukarch_pte_write);
}

static int _page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr,
	unsigned long flags, pte_write_func_t pte_write)
{
	__vaddr_t l1_table, l2_table, l3_table, l4_table, pte;
	__paddr_t pfn;
	size_t frame_size = PAGE_SIZE;
	int rc;

	if (!PAGE_ALIGNED(vaddr)) {
		uk_pr_err("Address must be aligned to page size\n");
		return -1;
	}

	l4_table = pt->pt_base;
	pte = ukarch_pte_read(l4_table, L4_OFFSET(vaddr), 4);
	if (!PAGE_PRESENT(pte))
		return -1;

	l3_table = (__vaddr_t) pt_pte_to_virt(pt, pte);
	pte = ukarch_pte_read(l3_table, L3_OFFSET(vaddr), 3);
	if (!PAGE_PRESENT(pte))
		return -1;

	l2_table = (__vaddr_t) pt_pte_to_virt(pt, pte);
	pte = ukarch_pte_read(l2_table, L2_OFFSET(vaddr), 2);
	if (!PAGE_PRESENT(pte))
		return -1;
	if (PAGE_LARGE(pte)) {
		if (!PAGE_LARGE_ALIGNED(vaddr))
			return -1;

		pfn = pte_to_pfn(pte);
		rc = pte_write(l2_table, L2_OFFSET(vaddr), 0, 2);
		if (rc)
			return -1;
		frame_size = PAGE_LARGE_SIZE;
	} else {
		l1_table = (unsigned long) pt_pte_to_virt(pt, pte);
		pte = ukarch_pte_read(l1_table, L1_OFFSET(vaddr), 1);
		if (!PAGE_PRESENT(pte))
			return -1;

		pfn = pte_to_pfn(pte);
		rc = pte_write(l1_table, L1_OFFSET(vaddr), 0, 1);
		if (rc)
			return -1;
		rc = uk_pt_release_if_unused(pt, vaddr, l1_table, l2_table, 1);
		if (rc)
			return -1;
	}

	ukarch_flush_tlb_entry(vaddr);

	if (pt->fa && !(flags & PAGE_FLAG_SHARED))
		pt->fa->pfree(pt->fa, pfn << PAGE_SHIFT, frame_size);

	rc = uk_pt_release_if_unused(pt, vaddr, l2_table, l3_table, 2);
	if (rc)
		return -1;
	rc = uk_pt_release_if_unused(pt, vaddr, l3_table, l4_table, 3);
	if (rc)
		return -1;

	return 0;
}

int ukplat_page_unmap(struct uk_pagetable *pt, __vaddr_t vaddr)
{
	return _page_unmap(pt, vaddr, 0, ukarch_pte_write);
}

int ukplat_page_unshare(struct uk_pagetable *pt, __vaddr_t vaddr)
{
	return _page_unmap(pt, vaddr, PAGE_FLAG_SHARED, ukarch_pte_write);
}

static int _map_region(struct uk_pagetable *pt, __vaddr_t vaddr,
	__paddr_t paddr, size_t pages, unsigned long prot,
	unsigned long flags, int is_initmem, pte_write_func_t pte_write)
{
	size_t i;
	unsigned long increment;
	int rc;

	if (flags & PAGE_FLAG_LARGE)
		increment = PAGE_LARGE_SIZE;
	else
		increment = PAGE_SIZE;

	for (i = 0; i < pages; i++) {
		unsigned long current_paddr;

		if (paddr == PAGE_PADDR_ANY)
			current_paddr = PAGE_PADDR_ANY;
		else
			current_paddr = pfn_to_mfn((paddr + i * increment) >> PAGE_SHIFT) << PAGE_SHIFT;

		rc = _page_map(pt, vaddr + i * increment, current_paddr, prot,
			flags, is_initmem, pte_write);
		if (rc) {
			size_t j;

			uk_pr_err("Could not map page 0x%08lx\n",
				vaddr + i * increment);

			for (j = 0; j < i; j++)
				_page_unmap(pt, vaddr, flags, pte_write);
			return -1;
		}
	}

	return 0;
}

int ukplat_page_map_many(struct uk_pagetable *pt, __vaddr_t vaddr,
		__paddr_t paddr, size_t pages, unsigned long prot,
		unsigned long flags)
{
	if (!pages)
		return 0;

	return _map_region(pt, vaddr, paddr, pages, prot, flags, 0,
			ukarch_pte_write);
}

int _initmem_page_map_many(struct uk_pagetable *pt, __vaddr_t vaddr,
	__paddr_t paddr, size_t pages, unsigned long prot, unsigned long flags)
{
	return _map_region(pt, vaddr, paddr, pages, prot, flags, 1,
		_ukarch_pte_write_raw);
}

__pte_t ukplat_virt_to_pte(struct uk_pagetable *pt, __vaddr_t vaddr)
{
	__pte_t pt_entry;
	__vaddr_t pt_vaddr;

	if (!PAGE_ALIGNED(vaddr)) {
		uk_pr_err("Address must be aligned to page size\n");
		return PAGE_NOT_MAPPED;
	}

	pt_vaddr = pt->pt_base;

	pt_entry = ukarch_pte_read(pt_vaddr, L4_OFFSET(vaddr), 4);
	if (!PAGE_PRESENT(pt_entry))
		return PAGE_NOT_MAPPED;

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);
	pt_entry = ukarch_pte_read(pt_vaddr, L3_OFFSET(vaddr), 3);
	if (!PAGE_PRESENT(pt_entry))
		return PAGE_NOT_MAPPED;
	if (PAGE_HUGE(pt_entry))
		return pt_entry;

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);
	pt_entry = ukarch_pte_read(pt_vaddr, L2_OFFSET(vaddr), 2);
	if (!PAGE_PRESENT(pt_entry))
		return PAGE_NOT_MAPPED;
	if (PAGE_LARGE(pt_entry))
		return pt_entry;

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);
	pt_entry = ukarch_pte_read(pt_vaddr, L1_OFFSET(vaddr), 1);

	return pt_entry;
}

struct uk_pagetable *uk_pt_init(__paddr_t paddr_start, size_t len)
{
	struct uk_pagetable *pt;
	size_t metadata_len;

	paddr_start = PAGE_ALIGN_UP(paddr_start);
	len = PAGE_ALIGN_DOWN(len);

	pt = &ukplat_default_pt;
	pt->pt_base = ukarch_read_pt_base();
	pt->pt_base_paddr = ukarch_read_pt_base();
	/* Page tables are mapped linearly in the default scenario */
	pt->virt_offset = 0;

	metadata_len = PAGE_ALIGN_UP(uk_frameallocbbuddy_metadata_size(paddr_start, len));
	/* TODO: this for loop is dependent on the hardcoded pagetable defined
	 * at plat/kvm/x86/pagetable.S. Maybe automate this so it still works
	 * if that pagetable changes. */
	for (__vaddr_t page = PAGE_LARGE_ALIGN_UP(paddr_start + metadata_len + PT_PAGES_CACHE_SIZE); page < 1024 * 1024 * 1024; page += PAGE_LARGE_SIZE)
		_page_unmap(pt, page, 0, ukarch_pte_write);

	pt->fa = uk_frameallocbbuddy_init(
			paddr_start + metadata_len + PT_PAGES_CACHE_SIZE,
			len - metadata_len - PT_PAGES_CACHE_SIZE,
			(void *) paddr_start);

	pt->pages_cache = (unsigned long *) (paddr_start + metadata_len);
	pt->pages_cache_length = PT_PAGES_CACHE_MAX_PAGES;

	for (size_t i = 0 ; i < PT_PAGES_CACHE_MAX_PAGES; i++)
		pt->pages_cache[i] = paddr_start + metadata_len + i * PAGE_SIZE;

	return pt;
}

void uk_pt_add_mem(__paddr_t paddr_start, size_t len)
{
	paddr_start = PAGE_ALIGN_UP(paddr_start);
	len = PAGE_ALIGN_DOWN(len);

	/* TODO */
}

/**
 * Create page tables that have mappings for themselves. Any other mappings
 * can be then created using the API, after the value returned by this function
 * is set as the PT base.
 * @return PT base, the physical address of the 4th level page table.
 */
static __paddr_t _pt_create(struct uk_framealloc *fa, unsigned long virt_offset)
{
	__vaddr_t pt_l4, pt_l3, pt_l2, pt_l1;
	size_t prev_l4_offset, prev_l3_offset, prev_l2_offset;
	__vaddr_t page;
	__paddr_t frame, paddr_pages;
	size_t used_pts = 0;

	UK_ASSERT(PAGE_ALIGNED(virt_offset));

	/* Allocate 7 pages for the worst case when, to map themselves, we need
	 * one L4, two L3s, two L2s and two L1s. */
	paddr_pages = fa->palloc(fa, 7);
	ukplat_page_map_many(ukplat_get_active_pt(), paddr_pages + virt_offset,
			paddr_pages, 7, PAGE_PROT_READ | PAGE_PROT_WRITE, 0);

	pt_l4 = paddr_pages + (used_pts++) * PAGE_SIZE;
	pt_l3 = paddr_pages + (used_pts++) * PAGE_SIZE;
	pt_l2 = paddr_pages + (used_pts++) * PAGE_SIZE;
	pt_l1 = paddr_pages + (used_pts++) * PAGE_SIZE;

	prev_l4_offset = L4_OFFSET(pt_l4 + virt_offset);
	prev_l3_offset = L3_OFFSET(pt_l4 + virt_offset);
	prev_l2_offset = L2_OFFSET(pt_l4 + virt_offset);

	_ukarch_pte_write_raw(pt_l4 + virt_offset, prev_l4_offset,
			(pfn_to_mfn(pt_l3 >> PAGE_SHIFT) << PAGE_SHIFT)
			| L4_PROT, 4);
	_ukarch_pte_write_raw(pt_l3 + virt_offset, prev_l3_offset,
			(pfn_to_mfn(pt_l2 >> PAGE_SHIFT) << PAGE_SHIFT)
			| L3_PROT, 3);
	_ukarch_pte_write_raw(pt_l2 + virt_offset, prev_l2_offset,
			(pfn_to_mfn(pt_l1 >> PAGE_SHIFT) << PAGE_SHIFT)
			| L2_PROT, 2);

	for (page = pt_l4 + virt_offset; page < pt_l4 + virt_offset + 7 * PAGE_SIZE; page += PAGE_SIZE) {
		if (L4_OFFSET(page) != prev_l4_offset) {
			pt_l3 = paddr_pages + (used_pts++) * PAGE_SIZE;
			_ukarch_pte_write_raw(pt_l4 + virt_offset,
				L4_OFFSET(page),
				(pfn_to_mfn(pt_l3 >> PAGE_SHIFT) << PAGE_SHIFT)
				| L4_PROT, 4);
			prev_l4_offset = L4_OFFSET(page);
		}

		if (L3_OFFSET(page) != prev_l3_offset) {
			pt_l2 = paddr_pages + (used_pts++) * PAGE_SIZE;
			_ukarch_pte_write_raw(pt_l3 + virt_offset,
				L3_OFFSET(page),
				(pfn_to_mfn(pt_l2 >> PAGE_SHIFT) << PAGE_SHIFT)
				| L3_PROT, 3);
			prev_l3_offset = L3_OFFSET(page);
		}

		if (L2_OFFSET(page) != prev_l2_offset) {
			pt_l1 = paddr_pages + (used_pts++) * PAGE_SIZE;
			_ukarch_pte_write_raw(pt_l2 + virt_offset,
				L2_OFFSET(page),
				(pfn_to_mfn(pt_l1 >> PAGE_SHIFT) << PAGE_SHIFT)
				| L2_PROT, 2);
			prev_l2_offset = L2_OFFSET(page);
		}

		frame = pfn_to_mfn((paddr_pages + page - pt_l4 - virt_offset) >> PAGE_SHIFT) << PAGE_SHIFT;
		_ukarch_pte_write_raw(pt_l1 + virt_offset, L1_OFFSET(page),
			frame | L1_PROT, 1);
	}

	return pt_l4;
}

static int _mmap_kernel(struct uk_pagetable *pt,
		__vaddr_t kernel_start_vaddr,
		__paddr_t kernel_start_paddr,
		size_t kernel_area_size)
{
	size_t kernel_pages;

	UK_ASSERT(PAGE_ALIGNED(kernel_start_vaddr));
	UK_ASSERT(PAGE_ALIGNED(kernel_start_paddr));

	if (ukplat_map_specific_areas(pt))
		return -1;

	/* TODO: break down into RW regions and RX regions */
	kernel_pages = DIV_ROUND_UP(kernel_area_size, PAGE_SIZE);
	if (_initmem_page_map_many(pt, kernel_start_vaddr,
			kernel_start_paddr, kernel_pages,
			PAGE_PROT_READ | PAGE_PROT_WRITE | PAGE_PROT_EXEC, 0))
		return -1;
	return 0;
}

/* TODO: write complementary function, that takes a page table and
 * destroys/releases all the pages used by it */
struct uk_pagetable *uk_pt_build(struct uk_framealloc *fa,
		__paddr_t kernel_paddr_start,
		__vaddr_t kernel_vaddr_start,
		size_t kernel_area_size)
{
	struct uk_pagetable *pt = calloc(1, sizeof(struct uk_pagetable));
	size_t metadata_len;

	UK_ASSERT(PAGE_ALIGNED(kernel_paddr_start));
	UK_ASSERT(PAGE_ALIGNED(kernel_vaddr_start));
	UK_ASSERT(PAGE_ALIGNED(kernel_area_size));

	pt->fa = fa;
	pt->pt_base_paddr = _pt_create(fa, DIRECTMAP_AREA_START);
	pt->pt_base = pt->pt_base_paddr + DIRECTMAP_AREA_START;
	pt->virt_offset = DIRECTMAP_AREA_START;
	pt->pages_cache = pt->fa->palloc(pt->fa, PT_PAGES_CACHE_SIZE >> PAGE_SHIFT) + DIRECTMAP_AREA_START;
	ukplat_page_map_many(ukplat_get_active_pt(), pt->pages_cache, (__paddr_t) pt->pages_cache - DIRECTMAP_AREA_START, PT_PAGES_CACHE_SIZE >> PAGE_SHIFT, PAGE_PROT_READ | PAGE_PROT_WRITE, 0);
	_refresh_pt_pages_cache(pt);
	ukplat_page_map_many(pt, pt->pages_cache, (__paddr_t) pt->pages_cache - DIRECTMAP_AREA_START, PT_PAGES_CACHE_SIZE >> PAGE_SHIFT, PAGE_PROT_READ | PAGE_PROT_WRITE, 0);

	if (_mmap_kernel(pt, kernel_vaddr_start, kernel_paddr_start,
				kernel_area_size))
		UK_CRASH("Could not map kernel\n");

	if (ukplat_mm_supported_features() & UKPLAT_READONLY_PAGETABLE) {
		/* TODO: if other page table pages are mapped here, make sure
		 * they are mapped as read-only if the platform requires it */
	}

	return pt;
}

static void dump_pt(struct uk_pagetable *pt, __vaddr_t vaddr)
{
	__pte_t pt_entry;
	__vaddr_t pt_vaddr;
	size_t i;

	if (!PAGE_ALIGNED(vaddr)) {
		uk_pr_err("Address must be aligned to page size\n");
		return;
	}

	pt_vaddr = pt->pt_base;

	printf("L4 table for address 0x%08lx is 0x%08lx\n", vaddr, pt_vaddr);
	for (i = 0; i < L4_OFFSET(vaddr) || i < 2; i += 2)
		printf("0x%08lx: 0x%08lx 0x%08lx\n", pt_vaddr + 8 * i,
				*((__vaddr_t *) pt_vaddr + i),
				*((__vaddr_t *) pt_vaddr + i + 1));

	pt_entry = ukarch_pte_read(pt_vaddr, L4_OFFSET(vaddr), 4);
	if (!PAGE_PRESENT(pt_entry))
		return;

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);

	printf("L3 table for address 0x%08lx is 0x%08lx\n", vaddr, pt_vaddr);
	for (i = 0; i < L3_OFFSET(vaddr) || i < 2; i += 2)
		printf("0x%08lx: 0x%08lx 0x%08lx\n", pt_vaddr + 8 * i,
				*((__vaddr_t *) pt_vaddr + i),
				*((__vaddr_t *) pt_vaddr + i + 1));

	pt_entry = ukarch_pte_read(pt_vaddr, L3_OFFSET(vaddr), 3);
	if (!PAGE_PRESENT(pt_entry))
		return;
	if (PAGE_HUGE(pt_entry)) {
		printf("PTE for vaddr 0x%08lx is 0x%08lx\n", vaddr, pt_entry);
		return;
	}

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);

	printf("L2 table for address 0x%08lx is 0x%08lx\n", vaddr, pt_vaddr);
	for (i = 0; i < L2_OFFSET(vaddr) || i < 2; i += 2)
		printf("0x%08lx: 0x%08lx 0x%08lx\n", pt_vaddr + 8 * i,
				*((__vaddr_t *) pt_vaddr + i),
				*((__vaddr_t *) pt_vaddr + i + 1));

	pt_entry = ukarch_pte_read(pt_vaddr, L2_OFFSET(vaddr), 2);
	if (!PAGE_PRESENT(pt_entry))
		return;
	if (PAGE_LARGE(pt_entry)) {
		printf("Large page PTE for vaddr 0x%08lx is 0x%08lx\n",
				vaddr, pt_entry);
		return;
	}

	pt_vaddr = (__vaddr_t) pt_pte_to_virt(pt, pt_entry);

	printf("L1 table for address 0x%08lx is 0x%08lx\n", vaddr, pt_vaddr);
	for (i = 0; i < L1_OFFSET(vaddr) || i < 2; i += 2)
		printf("0x%08lx: 0x%08lx 0x%08lx\n", pt_vaddr + 8 * i,
				*((__vaddr_t *) pt_vaddr + i),
				*((__vaddr_t *) pt_vaddr + i + 1));

	pt_entry = ukarch_pte_read(pt_vaddr, L1_OFFSET(vaddr), 1);

	printf("PTE for vaddr 0x%08lx is 0x%08lx\n", vaddr, pt_entry);
}

