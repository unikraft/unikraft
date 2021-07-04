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

/* TODO: there is currently just one page table used, Update accordingly when
 * using multiple page tables,
 */
struct uk_pagetable ukplat_default_pt;

struct uk_pagetable *ukplat_get_active_pt(void)
{
	return &ukplat_default_pt;
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

