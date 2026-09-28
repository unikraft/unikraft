/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight Copy-on-Write page fault handler.
 *
 * Hyperlight marks all writable guest pages as read-only with a
 * software CoW bit set (x86: PTE bit 9; arm64: descriptor bit 55).  A
 * write faults (x86: #PF; arm64: a permission-fault data abort); this
 * handler detects CoW pages, allocates a fresh page from scratch memory,
 * copies the content, and remaps as writable.
 *
 * Two CoW handlers exist during the guest lifecycle:
 *
 *   1. The early asm handler (x86/entry64.S, arm/entry64.S) — active
 *      from the entry code's lidt (x86) or VBAR write (arm64) until the
 *      native PAL's exception handling is installed, and again while a
 *      dispatch after a snapshot restore pre-faults the exception stacks.
 *
 *   2. This C handler — registered as a uk_event handler for
 *      UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT.  Once the native PAL's
 *      vectors are installed, page faults are dispatched through the
 *      event system and land here.
 *
 * hyperlight_cow_init() computes the scratch region base addresses; it
 * runs before that switch, while the asm handler still serves faults.
 */

#include <uk/assert.h>
#include <uk/essentials.h>
#if defined(__aarch64__)
#include <uk/arch/arm64.h>
#endif
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
 *   GPA: [HL_MAX_GPA - scratch_size + 1,  HL_MAX_GPA]   (top of guest RAM)
 *   GVA: [HL_MAX_GVA - scratch_size + 1,  HL_MAX_GVA]   (top of the address space; see mem.h)
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

/* The root page table's physical address. */
static inline __u64 cow_read_pt_root(void)
{
	__u64 val;

#if defined(__x86_64__)
	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val & ~0xFFFULL;
#elif defined(__aarch64__)
	__asm__ volatile("mrs %0, ttbr0_el1" : "=r"(val));
	return val & PTE_ADDR_MASK; /* drop ASID and CnP */
#endif
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

#if defined(__x86_64__)
	__asm__ volatile("lock xaddq %0, (%1)"
			 : "=r"(old)
			 : "r"(alloc_ptr), "0"(nbytes)
			 : "memory");
#elif defined(__aarch64__)
	/* An exclusive-monitor loop rather than LSE's ldadd: the image has
	 * to run on any ARMv8.0 core.
	 */
	old = __atomic_fetch_add(alloc_ptr, nbytes, __ATOMIC_SEQ_CST);
#endif

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
	__u64 table = cow_read_pt_root();
	__u64 entry;
	int shift;

	/* Walk the top three levels (x86 PML4 → PDPT → PD, arm64 L0 → L2) */
	for (shift = 39; shift > 12; shift -= 9) {
		__u64 idx = (addr >> shift) & 0x1FF;

		entry = cow_read_pte(table + idx * 8);
		if (!(entry & PTE_PRESENT))
			return 0;
#if defined(__aarch64__)
		/* A block descriptor: Hyperlight maps only 4 KiB pages */
		if (!(entry & 2))
			return 0;
#endif
		table = entry & PTE_ADDR_MASK;
	}

	/* Return address of the PT entry (level 12) */
	return table + ((__u64)((addr >> 12) & 0x1FF)) * 8;
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
#if defined(__x86_64__)
	unsigned long n = HL_PAGE_SIZE / 8;

	__asm__ volatile("rep movsq"
			 : "+D"(dst), "+S"(src), "+c"(n)
			 :
			 : "memory");
#elif defined(__aarch64__)
	/* This file is built -mgeneral-regs-only (it is an ISR source), so
	 * the loop cannot pick up SIMD registers either.
	 */
	__u64 *d = dst;
	const __u64 *s = src;
	unsigned long i;

	for (i = 0; i < HL_PAGE_SIZE / 8; i += 2) {
		__u64 a = s[i], b = s[i + 1];

		d[i] = a;
		d[i + 1] = b;
	}
#endif
}

/*
 * Point the leaf entry at @pte_addr, which maps @va, to @new_pte.  On
 * arm64 a valid descriptor cannot be changed to another output address
 * in place: the architecture requires break-before-make, invalidating the
 * old entry and its TLB entry first.  Nothing else can use the page
 * meanwhile, there is one vCPU and this runs with interrupts masked, but
 * this code itself must not touch it: the scratch bases the descriptor's
 * address is computed from are kernel data, possibly on that very page,
 * so the address is computed before the break.
 */
static inline void cow_set_pte(__u64 pte_addr, __u64 va, __u64 new_pte)
{
#if defined(__x86_64__)
	cow_write_pte(pte_addr, new_pte);
	__asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
#elif defined(__aarch64__)
	volatile __u64 *pte = (volatile __u64 *)cow_phys_to_virt(pte_addr);

	*pte = 0;
	__asm__ volatile("dsb ishst\n\t"
			 "tlbi vaae1is, %0\n\t"
			 "dsb ish"
			 : : "r"(va >> 12) : "memory");
	*pte = new_pte;
	__asm__ volatile("dsb ishst\n\t"
			 "isb" : : : "memory");
#endif
}

/* The descriptor bits that make a read-only page writable. */
static inline __u64 cow_pte_writable(__u64 pte)
{
#if defined(__x86_64__)
	return pte | PTE_RW;
#elif defined(__aarch64__)
	return pte & ~PTE_AP_RO;
#endif
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

#if defined(__x86_64__)
	/*
	 * Must be: present + write + supervisor + not insn-fetch + not rsvd.
	 * Bits: 0=present, 1=write, 2=user, 3=rsvd, 4=insn-fetch
	 */
	if ((error_code & 0x1F) != 0x03)
		return 0;
#elif defined(__aarch64__)
	/*
	 * Must be a data abort taken from EL1 on a level-3 permission fault,
	 * caused by a write (ISS.WnR), and not by a cache maintenance
	 * instruction (ISS.CM), which reports itself as a write too.
	 */
	if (UK_ARCH_ARM64_ESR_EC_FROM(error_code) !=
	    UK_ARCH_ARM64_ESR_EL1_EC_MMU_DABRT_EL1)
		return 0;
	if (UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_FROM(error_code) !=
	    UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_PERM_L3)
		return 0;
	if (!(error_code & (1UL << 6)) || (error_code & (1UL << 8)))
		return 0;
#endif

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
	new_pte = cow_pte_writable(new_gpa |
				   (pte & ~(PTE_ADDR_MASK | PTE_AVL_COW)));
	cow_set_pte(pte_addr, page_base, new_pte);
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
	cow_set_pte(pte_addr, va & ~(HL_PAGE_SIZE - 1),
		    cow_pte_writable(new_gpa |
				     (pte & ~(PTE_ADDR_MASK | PTE_AVL_COW))));
}

/*
 * uk_event handler for page faults.  Registered at UK_PRIO_EARLIEST
 * so CoW faults are resolved before any other handler sees them.
 */
static int hyperlight_cow_pf_handler(void *data)
{
	struct uk_lcpu_except_err_ctx *ctx = data;
	__vaddr_t fault_addr;
#if defined(__x86_64__)
	int error_code;
#elif defined(__aarch64__)
	__u64 error_code;
#endif

	if (!cow_initialized)
		return UK_EVENT_NOT_HANDLED;

	fault_addr = (__vaddr_t)uk_lcpu_except_err_ctx_get_fault_addr(ctx);
#if defined(__x86_64__)
	error_code = uk_lcpu_x86_64_except_err_ctx_get_error_code(ctx);
#elif defined(__aarch64__)
	error_code = uk_lcpu_arm64_except_err_ctx_get_esr(ctx);
#endif

	if (cow_handle_fault(fault_addr, (__u64)error_code))
		return UK_EVENT_HANDLED;

	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      hyperlight_cow_pf_handler, UK_PRIO_EARLIEST);

/*
 * Initialise the C CoW handler.  Called from setup.c before
 * uk_lcpu_init() installs the native PAL's vectors, while the early asm
 * handler still serves page faults: the writes below may themselves
 * fault on a CoW page, which the C handler could not serve before
 * cow_initialized is set.
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
