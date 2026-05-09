/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Hyperlight Copy-on-Write page fault handler
 *
 * When Hyperlight maps writable guest pages as CoW, it marks PTE bit 9
 * (PAGE_AVL_COW) and clears the write bit. On write access, a #PF occurs.
 * This handler detects CoW pages, allocates a fresh page from scratch
 * memory, copies the content, and remaps as writable.
 *
 * Copyright (c) 2024 Microsoft Corporation
 */

#include <uk/essentials.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <string.h>

/* Hyperlight scratch memory layout constants (amd64) */
#define HL_MAX_GPA          0x0000000FFFFFFFFFULL
#define HL_MAX_GVA          0xFFFFFFFFFFFFEFFFULL

/* PTE bit definitions */
#define PTE_PRESENT         (1ULL << 0)
#define PTE_RW              (1ULL << 1)
#define PTE_NX              (1ULL << 63)
#define PTE_AVL_COW         (1ULL << 9)
#define PTE_ADDR_MASK       0x000FFFFFFFFFF000ULL

#define HL_PAGE_SIZE        4096ULL

/* Scratch metadata addresses (at top of scratch region) */
#define SCRATCH_SIZE_GVA    (HL_MAX_GVA - 0x08ULL + 1ULL)  /* 0xFFFFFFFFFFFFEFF8 */
#define ALLOCATOR_GVA       (HL_MAX_GVA - 0x10ULL + 1ULL)  /* 0xFFFFFFFFFFFFEFF0 */

static __u64 cow_scratch_base_gpa;
static __u64 cow_scratch_base_gva;
static int   cow_initialized;

static inline __u64 cow_phys_to_virt(__u64 gpa)
{
	return cow_scratch_base_gva + (gpa - cow_scratch_base_gpa);
}

static inline __u64 cow_read_cr3(void)
{
	__u64 val;

	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val & ~0xFFFULL;
}

/* Read a page table entry given its physical address */
static inline __u64 cow_read_pte(__u64 pte_phys_addr)
{
	__u64 val;
	void *virt = (void *)cow_phys_to_virt(pte_phys_addr);

	__asm__ volatile("movq (%1), %0"
			 : "=r"(val) : "r"(virt) : "memory");
	return val;
}

/* Write a page table entry given its physical address */
static inline void cow_write_pte(__u64 pte_phys_addr, __u64 value)
{
	void *virt = (void *)cow_phys_to_virt(pte_phys_addr);

	__asm__ volatile("movq %1, (%0)"
			 : : "r"(virt), "r"(value) : "memory");
}

/* Bump allocator: allocate n pages from scratch memory.
 * Returns physical address of the allocated region.
 */
static __u64 cow_alloc_phys_pages(__u64 n)
{
	volatile __u64 *alloc_ptr = (volatile __u64 *)ALLOCATOR_GVA;
	__u64 nbytes = n * HL_PAGE_SIZE;
	__u64 old_offset;

	__asm__ volatile("lock xaddq %0, (%1)"
			 : "=r"(old_offset)
			 : "r"(alloc_ptr), "0"(nbytes)
			 : "memory");

	return old_offset;
}

/*
 * Walk 4-level page tables to find the physical address of the PTE
 * for a given guest virtual address.
 * Returns 0 if any level is not present.
 */
static __u64 cow_walk_pt_for_pte_addr(__u64 gva)
{
	__u64 pml4_base = cow_read_cr3();
	__u64 addr, pml4_idx, pdpt_idx, pd_idx, pt_idx;
	__u64 pml4e, pdpt_base, pdpte, pd_base, pde, pt_base;

	/* Undo sign extension for index calculation */
	addr = gva & ((1ULL << 48) - 1);

	pml4_idx = (addr >> 39) & 0x1FF;
	pdpt_idx = (addr >> 30) & 0x1FF;
	pd_idx   = (addr >> 21) & 0x1FF;
	pt_idx   = (addr >> 12) & 0x1FF;

	/* PML4 -> PDPT */
	pml4e = cow_read_pte(pml4_base + pml4_idx * 8);
	if (!(pml4e & PTE_PRESENT))
		return 0;

	/* PDPT -> PD */
	pdpt_base = pml4e & PTE_ADDR_MASK;
	pdpte = cow_read_pte(pdpt_base + pdpt_idx * 8);
	if (!(pdpte & PTE_PRESENT))
		return 0;

	/* PD -> PT */
	pd_base = pdpte & PTE_ADDR_MASK;
	pde = cow_read_pte(pd_base + pd_idx * 8);
	if (!(pde & PTE_PRESENT))
		return 0;

	/* Return physical address of the PT entry */
	pt_base = pde & PTE_ADDR_MASK;
	return pt_base + pt_idx * 8;
}

/*
 * Handle a Copy-on-Write page fault.
 *
 * Returns 1 if the fault was handled (CoW copy performed),
 * 0 if the fault is not a CoW fault.
 */
int cow_handle_fault(__u64 fault_addr, unsigned long error_code,
		     __u64 faulting_rip __unused)
{
	__u64 pte_addr, pte;
	__u64 new_page_gpa, new_page_gva;
	__u64 page_start;
	__u64 old_flags, new_pte;

	/* Check: present + write + not user + not insn fetch + not rsvd */
	if (!(error_code & (1 << 0)))   /* Not present */
		return 0;
	if (!(error_code & (1 << 1)))   /* Not a write */
		return 0;
	if (error_code & (1 << 2))      /* User mode */
		return 0;
	if (error_code & (1 << 4))      /* Instruction fetch */
		return 0;
	if (error_code & (1 << 3))      /* Reserved bit */
		return 0;

	/* Walk page tables to find the PTE */
	pte_addr = cow_walk_pt_for_pte_addr(fault_addr);
	if (!pte_addr)
		return 0;

	pte = cow_read_pte(pte_addr);

	/* Check CoW bit (bit 9) */
	if (!(pte & PTE_AVL_COW))
		return 0;

	/* Allocate new page from scratch and copy content */
	new_page_gpa = cow_alloc_phys_pages(1);
	new_page_gva = cow_phys_to_virt(new_page_gpa);
	page_start = fault_addr & ~(HL_PAGE_SIZE - 1);
	memcpy((void *)new_page_gva, (void *)page_start, HL_PAGE_SIZE);

	/* Build new PTE: writable, no CoW bit, new physical address.
	 * Preserve the original NX setting — only pages that were
	 * executable before CoW should remain executable after.
	 */
	old_flags = pte & ~(PTE_ADDR_MASK | PTE_AVL_COW);
	new_pte = new_page_gpa | old_flags | PTE_RW;

	cow_write_pte(pte_addr, new_pte);

	/* Invalidate TLB for this page */
	__asm__ volatile("invlpg (%0)" : : "r"(page_start) : "memory");

	return 1;
}

/*
 * Demand-page a virtual address: walk the page tables and create entries
 * as needed, allocating page table pages and data pages from scratch.
 * Used by mmap to back virtual-only PROT_NONE reservations when Go
 * commits sub-regions with MAP_FIXED.
 *
 * Returns 1 on success, 0 if CoW is not initialized.
 */
int cow_demand_map_page(__u64 gva)
{
	__u64 cr3, addr;
	__u64 pml4_idx, pdpt_idx, pd_idx, pt_idx;
	__u64 entry_addr, entry_val;
	__u64 next_base;

	if (!cow_initialized)
		return 0;

	cr3 = cow_read_cr3();
	addr = gva & ((1ULL << 48) - 1);

	pml4_idx = (addr >> 39) & 0x1FF;
	pdpt_idx = (addr >> 30) & 0x1FF;
	pd_idx   = (addr >> 21) & 0x1FF;
	pt_idx   = (addr >> 12) & 0x1FF;

	/* PML4 → PDPT */
	entry_addr = cr3 + pml4_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	/* PDPT → PD */
	entry_addr = next_base + pdpt_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	/* PD → PT */
	entry_addr = next_base + pd_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	/* PT → Data page */
	entry_addr = next_base + pt_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
	}
	/* If already present+RW, nothing to do.
	 * If present but CoW (read-only), leave it — the CoW handler
	 * will resolve it on the first write.
	 */

	/* Flush TLB for this page */
	{
		__u64 page_start = gva & ~(HL_PAGE_SIZE - 1);
		__asm__ volatile("invlpg (%0)" : : "r"(page_start) : "memory");
	}

	return 1;
}

/*
 * Mapped file region for demand-paging.
 * Set by the guest when it discovers a map_file_cow mapping.
 */
static __u64 mapped_file_base;
static __u64 mapped_file_size;

void cow_register_mapped_file(__u64 base, __u64 size)
{
	mapped_file_base = base;
	mapped_file_size = size;
}

/*
 * Demand-page a file-mapped region: create page table entries pointing
 * to the identity-mapped physical address (GVA == GPA).
 * The KVM memory slot already has the file data; we just need PTEs.
 * Maps read-only (the file data should not be modified).
 */
static int cow_demand_map_file_page(__u64 gva)
{
	__u64 cr3, addr;
	__u64 pml4_idx, pdpt_idx, pd_idx, pt_idx;
	__u64 entry_addr, entry_val;
	__u64 next_base;
	__u64 page_gpa;

	if (!cow_initialized)
		return 0;
	if (!mapped_file_base || !mapped_file_size)
		return 0;
	if (gva < mapped_file_base || gva >= mapped_file_base + mapped_file_size)
		return 0;

	/* Identity mapping: GPA == GVA */
	page_gpa = gva & ~(HL_PAGE_SIZE - 1);

	cr3 = cow_read_cr3();
	addr = gva & ((1ULL << 48) - 1);

	pml4_idx = (addr >> 39) & 0x1FF;
	pdpt_idx = (addr >> 30) & 0x1FF;
	pd_idx   = (addr >> 21) & 0x1FF;
	pt_idx   = (addr >> 12) & 0x1FF;

	/* Walk/create PML4 → PDPT → PD → PT, allocating table pages as needed */
	entry_addr = cr3 + pml4_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	entry_addr = next_base + pdpt_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	entry_addr = next_base + pd_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		__u64 pg = cow_alloc_phys_pages(1);
		__u64 va = cow_phys_to_virt(pg);
		__builtin_memset((void *)va, 0, HL_PAGE_SIZE);
		cow_write_pte(entry_addr, pg | PTE_PRESENT | PTE_RW);
		next_base = pg;
	} else {
		next_base = entry_val & PTE_ADDR_MASK;
	}

	/* Map the data page: read-only, pointing to the file's physical page */
	entry_addr = next_base + pt_idx * 8;
	entry_val = cow_read_pte(entry_addr);
	if (!(entry_val & PTE_PRESENT)) {
		cow_write_pte(entry_addr, page_gpa | PTE_PRESENT);
	}

	return 1;
}

/*
 * Event handler for page faults (new ukpal/uklcpu API).
 *
 * After the C exception handlers are installed, CoW faults are dispatched
 * via the platform's page fault event.  This handler intercepts them before
 * any other handler (UK_PRIO_EARLIEST) and resolves CoW faults using
 * cow_handle_fault.  Non-CoW faults return UK_EVENT_NOT_HANDLED so that
 * the rest of the event chain can process them.
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

	/* Try CoW resolution first (present + write faults) */
	if (cow_handle_fault(fault_addr, (__u64)error_code, 0))
		return UK_EVENT_HANDLED;

	/* Try demand-paging for file-mapped regions (not-present faults) */
	if (!(error_code & 1) && cow_demand_map_file_page(fault_addr))
		return UK_EVENT_HANDLED;

	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      hyperlight_cow_pf_handler, UK_PRIO_EARLIEST);

/*
 * Initialize the CoW handler.
 * Must be called early in boot, before any CoW pages are accessed
 * through the C-level handler (the asm handler in entry64.S handles
 * CoW before this runs).
 */
void hyperlight_cow_init(void)
{
	__u64 scratch_size;

	scratch_size = *(volatile __u64 *)SCRATCH_SIZE_GVA;
	if (scratch_size == 0)
		return;

	cow_scratch_base_gpa = HL_MAX_GPA - scratch_size + 1;
	cow_scratch_base_gva = HL_MAX_GVA - scratch_size + 1;
	cow_initialized = 1;
}
