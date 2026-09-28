/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight Copy-on-Write page fault handler.
 *
 * Hyperlight marks all writable guest pages as read-only with PTE
 * bit 9 (AVL/CoW) set.  A write triggers #PF; this handler detects
 * CoW pages, allocates a fresh page from scratch memory, copies the
 * content, and remaps as writable.
 *
 * Two CoW handlers exist during the guest lifecycle:
 *
 *   1. The asm handler in entry64.S — active from the very first
 *      instruction until uk_lcpu_init() replaces the minimal IDT
 *      with the full native PAL IDT.
 *
 *   2. This C handler — registered as a uk_event handler for
 *      UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT.  After uk_lcpu_init()
 *      installs the native PAL's IDT, page faults are dispatched
 *      through the event system and land here.
 *
 * hyperlight_cow_init() must be called after uk_lcpu_init() to
 * compute the scratch region base addresses.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/lcpu.h>

#include <hyperlight/mem.h>

#define HL_PAGE_SIZE	4096ULL

/*
 * Scratch region base addresses, computed at init from scratch metadata.
 * Non-static so the paging arch layer can translate scratch GPAs to GVAs.
 */
__u64 hl_scratch_base_gpa;
__u64 hl_scratch_base_gva;

static int cow_initialized;

/*
 * Convert a guest physical address (GPA) in the scratch region to
 * the corresponding guest virtual address (GVA).
 *
 * The scratch region is NOT identity-mapped.  The host maps it at:
 *   GPA: [HL_MAX_GPA - scratch_size + 1,  HL_MAX_GPA]   (40-bit top)
 *   GVA: [HL_MAX_GVA - scratch_size + 1,  HL_MAX_GVA]   (canonical top)
 *
 * The mapping is contiguous and linear, so the offset between GVA
 * and GPA is constant: (cow_scratch_base_gva - cow_scratch_base_gpa).
 *
 * Page table pages live in the scratch region, so
 * every PTE the CoW handler reads contains a GPA that needs this
 * conversion to be dereferenced.
 */
static inline __u64 cow_phys_to_virt(__u64 gpa)
{
	return hl_scratch_base_gva + (gpa - hl_scratch_base_gpa);
}

static inline __u64 cow_read_cr3(void)
{
	__u64 val;

	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val & ~0xFFFULL;
}

/* Read/write a PTE by its physical address (via scratch GPA→GVA offset) */
static inline __u64 cow_read_pte(__u64 pte_phys)
{
	return *(volatile __u64 *)cow_phys_to_virt(pte_phys);
}

static inline void cow_write_pte(__u64 pte_phys, __u64 value)
{
	*(volatile __u64 *)cow_phys_to_virt(pte_phys) = value;
}

/*
 * Bump allocator: allocate n pages from scratch memory (atomic).
 * Returns the GPA of the first page.  Aborts on scratch exhaustion
 * (matches hyperlight-guest-bin behaviour).
 *
 * Two pages at the top of scratch are reserved for the exception
 * stack and shared metadata — the allocator must not touch them.
 */
#define HL_SCRATCH_RESERVED_PAGES 2
__u64 hl_scratch_alloc_pages(__u64 n)
{
	volatile __u64 *alloc_ptr = (volatile __u64 *)HL_SCRATCH_ALLOC_GVA;
	__u64 nbytes = n * HL_PAGE_SIZE;
	__u64 old;
	__u64 max_avail = HL_MAX_GPA + 1 - HL_SCRATCH_RESERVED_PAGES * HL_PAGE_SIZE;

	__asm__ volatile("lock xaddq %0, (%1)"
			 : "=r"(old)
			 : "r"(alloc_ptr), "0"(nbytes)
			 : "memory");

	if (old + nbytes > max_avail)
		UK_CRASH("Out of scratch memory: need %lu bytes at %lx, max %lx\n",
			 (unsigned long)nbytes, (unsigned long)old,
			 (unsigned long)max_avail);

	return old;
}

/*
 * Walk 4-level page tables to find the physical address of the
 * leaf PTE for a given virtual address.  Returns 0 if any level
 * is not present.
 */
static __u64 cow_walk_to_pte(__u64 gva)
{
	__u64 addr = gva & ((1ULL << 48) - 1);
	__u64 cr3 = cow_read_cr3();
	__u64 entry;
	int shift;

	/* Walk PML4 → PDPT → PD (levels 39, 30, 21) */
	for (shift = 39; shift > 12; shift -= 9) {
		__u64 idx = (addr >> shift) & 0x1FF;

		entry = cow_read_pte(cr3 + idx * 8);
		if (!(entry & PTE_PRESENT))
			return 0;
		cr3 = entry & PTE_ADDR_MASK;
	}

	/* Return address of the PT entry (level 12) */
	return cr3 + ((__u64)((addr >> 12) & 0x1FF)) * 8;
}

/*
 * Copy a page without SSE registers: the exception entry saves only the
 * general-purpose ones, so an SSE register this handler changed would
 * reach the code that faulted — typically mid-way through a vectorised
 * store, the very write that raised the fault.  memcpy() uses them, and
 * memcpy_isr() copies a byte at a time.
 */
static inline void cow_copy_page(void *dst, const void *src)
{
	unsigned long n = HL_PAGE_SIZE / 8;

	__asm__ volatile("rep movsq"
			 : "+D"(dst), "+S"(src), "+c"(n)
			 :
			 : "memory");
}

/*
 * Handle a CoW page fault.
 * Returns 1 if resolved (CoW copy performed), 0 if not a CoW fault.
 */
static int cow_handle_fault(__u64 fault_addr, unsigned long error_code)
{
	__u64 pte_addr, pte;
	__u64 new_gpa, new_gva;
	__u64 page_base, new_pte;

	/*
	 * Must be: present + write + supervisor + not insn-fetch + not rsvd.
	 * Bits: 0=present, 1=write, 2=user, 3=rsvd, 4=insn-fetch
	 */
	if ((error_code & 0x1F) != 0x03)
		return 0;

	pte_addr = cow_walk_to_pte(fault_addr);
	if (!pte_addr)
		return 0;

	pte = cow_read_pte(pte_addr);
	if (!(pte & PTE_AVL_COW))
		return 0;

	/* Allocate, copy, remap */
	new_gpa = hl_scratch_alloc_pages(1);
	new_gva = cow_phys_to_virt(new_gpa);
	page_base = fault_addr & ~(HL_PAGE_SIZE - 1);
	cow_copy_page((void *)new_gva, (const void *)page_base);

	/* New PTE: writable, no CoW bit, new physical address.
	 * Preserve NX — only pages executable before remain so after.
	 */
	new_pte = new_gpa | (pte & ~(PTE_ADDR_MASK | PTE_AVL_COW)) | PTE_RW;
	cow_write_pte(pte_addr, new_pte);

	__asm__ volatile("invlpg (%0)" : : "r"(page_base) : "memory");
	return 1;
}

/*
 * Give a copy-on-write page a fresh scratch page without copying it, for
 * memory whose contents do not matter.  Unlike a copy, this does not
 * touch the new page: the host backs it only when the guest first writes
 * it, so memory that is rarely used (most of an exception stack) costs
 * nothing on a restore.
 */
void hyperlight_cow_fresh(__u64 va)
{
	__u64 pte_addr = cow_walk_to_pte(va);
	__u64 pte, new_gpa;

	if (!pte_addr)
		return;
	pte = cow_read_pte(pte_addr);
	if (!(pte & PTE_PRESENT) || !(pte & PTE_AVL_COW))
		return;

	new_gpa = hl_scratch_alloc_pages(1);
	cow_write_pte(pte_addr, new_gpa |
		      (pte & ~(PTE_ADDR_MASK | PTE_AVL_COW)) | PTE_RW);
	__asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
}

/*
 * uk_event handler for page faults.  Registered at UK_PRIO_EARLIEST
 * so CoW faults are resolved before any other handler sees them.
 */
static int hyperlight_cow_pf_handler(void *data)
{
	struct uk_lcpu_except_err_ctx *ctx = data;
	__vaddr_t fault_addr;
	int error_code;

	if (!cow_initialized)
		return UK_EVENT_NOT_HANDLED;

	fault_addr = (__vaddr_t)uk_lcpu_except_err_ctx_get_fault_addr(ctx);
	error_code = uk_lcpu_x86_64_except_err_ctx_get_error_code(ctx);

	if (cow_handle_fault(fault_addr, (__u64)error_code))
		return UK_EVENT_HANDLED;

	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      hyperlight_cow_pf_handler, UK_PRIO_EARLIEST);

/*
 * Initialise the C CoW handler.  Called from setup.c immediately
 * after uk_lcpu_init() returns.
 *
 * Between lidt (inside uk_lcpu_init) and cow_initialized = 1 here,
 * the C handler is registered but not yet initialised: any CoW fault
 * in that window would go unhandled.  This is safe because all BSS
 * writes (GDT/TSS/IDT structures) happen before lidt, and the
 * return path back to this call touches no new CoW pages (the stack
 * is already faulted in).
 */
void hyperlight_cow_init(void)
{
	__u64 scratch_size;

	scratch_size = *(volatile __u64 *)HL_SCRATCH_SIZE_GVA;
	if (scratch_size == 0)
		return;

	hl_scratch_base_gpa = HL_MAX_GPA - scratch_size + 1;
	hl_scratch_base_gva = HL_MAX_GVA - scratch_size + 1;
	cow_initialized = 1;
}
