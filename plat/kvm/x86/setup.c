/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <uk/plat/common/acpi.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/arch/paging.h>
#include <uk/asm/cfi.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/intctlr.h>

#include <kvm/console.h>
#ifdef CONFIG_RUNTIME_ASLR
#include <uk/swrand.h>
#endif

#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>

#ifdef CONFIG_HAVE_PAGING
#include <uk/plat/paging.h>
#include <uk/falloc.h>
#endif /* CONFIG_HAVE_PAGING */

#define PLATFORM_MAX_MEM_ADDR 0x100000000 /* 4 GiB */

/**
 * Allocates page-aligned memory by taking it away from the free physical
 * memory. Only memory in the first 4 GiB is used so that it is accessible also
 * with the static 1:1 boot page table. Note, the memory cannot be released!
 *
 * @param size
 *   The size to allocate. Will be rounded up to next multiple of page size.
 * @param type
 *   Memory region type to use for the allocated memory. Can be 0.
 *
 * @return
 *   A pointer to the allocated memory on success, NULL otherwise
 */

// static inline void _init_mem(struct uk_bootinfo *bi)
// {
// #ifdef CONFIG_RUNTIME_ASLR
// 	int ASLR_offset;
// 	int divisor = 1;
// 	/*
// 	 * Since we can't modify __STACK_SIZE which is a macro, we place first the 
// 	 * stack and then the heap. 
// 	 */
// 	ASLR_offset = uk_swrand_randr() % (bi->max_addr/4);
// 	_libkvmplat_cfg.bstack.end = ALIGN_UP(bi->max_addr - ASLR_offset, __PAGE_SIZE);
// 	_libkvmplat_cfg.bstack.start   = _libkvmplat_cfg.bstack.end-__STACK_SIZE;
// 	_libkvmplat_cfg.bstack.len   = __STACK_SIZE;
	
// 	uk_pr_info(" ASLR - Stack : s: %p e: %p\n",_libkvmplat_cfg.bstack.start,
// 	_libkvmplat_cfg.bstack.end);
	
// 	do{
// 	ASLR_offset = uk_swrand_randr() % (bi->max_addr/(4*divisor));
// 	_libkvmplat_cfg.heap.start = ALIGN_UP((uintptr_t) __END+ASLR_offset, __PAGE_SIZE);
// 	divisor++;
// 	}
// 	while(_libkvmplat_cfg.heap.start >= _libkvmplat_cfg.bstack.start);

// 	_libkvmplat_cfg.heap.end = (uintptr_t) _libkvmplat_cfg.bstack.start;
// 	_libkvmplat_cfg.heap.len   = _libkvmplat_cfg.heap.end
// 				     - _libkvmplat_cfg.heap.start;
// 	uk_pr_info(" ASLR - Heap : s: %p e: %p len: %p\n",_libkvmplat_cfg.heap.start,
// 	_libkvmplat_cfg.heap.end,_libkvmplat_cfg.heap.len );
				 
	
	
// #else
// 	_libkvmplat_cfg.heap.start = ALIGN_UP((uintptr_t)__END, __PAGE_SIZE);
// 	_libkvmplat_cfg.heap.end = (uintptr_t)bi->max_addr - __STACK_SIZE;
// 	_libkvmplat_cfg.heap.len =
// 	    _libkvmplat_cfg.heap.end - _libkvmplat_cfg.heap.start;
// 	_libkvmplat_cfg.bstack.start = _libkvmplat_cfg.heap.end;
// 	_libkvmplat_cfg.bstack.end = bi->max_addr;
// 	_libkvmplat_cfg.bstack.len = __STACK_SIZE;
// #endif
// }

static void *bootmemory_palloc(__sz size, int type)
{
	struct ukplat_memregion_desc *mrd;
	__paddr_t pstart, pend;
	__paddr_t ostart, olen;
	int rc;

	size = PAGE_ALIGN_UP(size);
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT(mrd->pbase <= __U64_MAX - size);
		pstart = PAGE_ALIGN_UP(mrd->pbase);
		pend   = pstart + size;

		if (pend > PLATFORM_MAX_MEM_ADDR ||
		    pend > mrd->pbase + mrd->len)
			continue;

		UK_ASSERT((mrd->flags & UKPLAT_MEMRF_PERMS) ==
			  (UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE));

		ostart = mrd->pbase;
		olen   = mrd->len;

		/* Adjust free region */
		mrd->len  -= pend - mrd->pbase;
		mrd->pbase = pend;

		mrd->vbase = (__vaddr_t)mrd->pbase;

		/* Insert allocated region */
		rc = ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds,
			&(struct ukplat_memregion_desc){
				.vbase = pstart,
				.pbase = pstart,
				.len   = size,
				.type  = type,
				.flags = UKPLAT_MEMRF_READ |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP,
			});
		if (unlikely(rc < 0)) {
			/* Restore original region */
			mrd->vbase = ostart;
			mrd->len   = olen;

			return NULL;
		}

		return (void *)pstart;
	}

	return NULL;
}

#ifdef CONFIG_RUNTIME_ASLR
static void *stackmemory_aslr_palloc(__sz size) {
	struct ukplat_memregion_desc *mrd;
	__paddr_t pstart, pend;
	__paddr_t tmp_pstart, tmp_len;
	int rc_b, rc_s, rc_a;
	unsigned long ASLR_offset;
	struct ukplat_memregion_desc old_mrd;

	size = PAGE_ALIGN_UP(size);
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		UK_ASSERT(mrd->pbase <= __U64_MAX - size);
		pstart = PAGE_ALIGN_UP(mrd->pbase);
		pend   = pstart + size;

		if (pend > PLATFORM_MAX_MEM_ADDR ||
		    pend > mrd->pbase + mrd->len)
			continue;

		UK_ASSERT((mrd->flags & UKPLAT_MEMRF_PERMS) ==
			  (UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE));
		
		old_mrd.vbase = mrd->vbase;
		old_mrd.pbase = mrd->pbase;
		old_mrd.len   = mrd->len;
		old_mrd.type  = UKPLAT_MEMRT_FREE;
		old_mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE | UKPLAT_MEMRF_MAP;

		// TODO:
		// 3. Check if the this approach "%" is fine
		// 4. How can I see the start of the heap address?
		ASLR_offset = uk_swrand_randr() % (mrd->len - size);
		uk_pr_info("ASLR_offset: %x\n", ASLR_offset);
		uk_pr_info("pbase: %x\n", mrd->pbase);
		uk_pr_info("stack start: %x\n", PAGE_ALIGN_UP(mrd->pbase + ASLR_offset));
		uk_pr_info("pend: %x\n", PAGE_ALIGN_UP(mrd->pbase + ASLR_offset + size));
		pstart = PAGE_ALIGN_UP(mrd->pbase + ASLR_offset);
		pend = PAGE_ALIGN_UP(pstart + size);

		tmp_pstart = mrd->pbase;
		tmp_len = mrd->len;

		/* Delete the old memory region to make space for the three new regions (before stack, stack, after stack) */
		ukplat_memregion_list_delete(&ukplat_bootinfo_get()->mrds, __ukplat_memregion_foreach_i);

		// Insert the free region before the stack start address
		rc_b = ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds,
			&(struct ukplat_memregion_desc){
				.vbase = PAGE_ALIGN_UP(tmp_pstart),
				.pbase = PAGE_ALIGN_UP(tmp_pstart),
				.len   = PAGE_ALIGN_UP(ASLR_offset),
				.type  = UKPLAT_MEMRT_FREE,
				.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE |
					UKPLAT_MEMRF_MAP,
			});
		if (unlikely(rc_b < 0)) {
			/* Restore original region */
			ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds, &old_mrd);

			return NULL;
		}

		/* Insert the stack region */
		rc_s = ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds,
			&(struct ukplat_memregion_desc){
				.vbase = pstart,
				.pbase = pstart,
				.len   = size,
				.type  = UKPLAT_MEMRT_STACK,
				.flags = UKPLAT_MEMRF_READ |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP,
			});
		if (unlikely(rc_s < 0)) {
			/* Delete previously inserted region */
			ukplat_memregion_list_delete(&ukplat_bootinfo_get()->mrds, rc_b);

			/* Restore original region */
			ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds, &old_mrd);

			return NULL;
		}

		/* Adjust the len and start of the after the stack region */
		tmp_len  = tmp_pstart + tmp_len - pend;
		tmp_pstart = pend;

		// Insert the free region after the stack start address
		rc_a = ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds,
			&(struct ukplat_memregion_desc){
				.vbase = PAGE_ALIGN_UP(tmp_pstart),
				.pbase = PAGE_ALIGN_UP(tmp_pstart),
				.len   = PAGE_ALIGN_UP(tmp_len),
				.type  = UKPLAT_MEMRT_FREE,
				.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE |
					UKPLAT_MEMRF_MAP,
			});
		if (unlikely(rc_a < 0)) {
			/* Delete previously inserted region */
			ukplat_memregion_list_delete(&ukplat_bootinfo_get()->mrds, rc_b);
			ukplat_memregion_list_delete(&ukplat_bootinfo_get()->mrds, rc_s);

			/* Restore original region */
			ukplat_memregion_list_insert(&ukplat_bootinfo_get()->mrds, &old_mrd);

			return NULL;
		}

		return (void *)pstart;
	}

	return NULL;
}
#endif

#ifdef CONFIG_HAVE_PAGING
/* Initial page table struct used for paging API to absorb statically defined
 * startup page table.
 */
static struct uk_pagetable kernel_pt;

static inline unsigned long bootinfo_to_page_attr(__u16 flags)
{
	unsigned long prot = 0;

	if (flags & UKPLAT_MEMRF_READ)
		prot |= PAGE_ATTR_PROT_READ;
	if (flags & UKPLAT_MEMRF_WRITE)
		prot |= PAGE_ATTR_PROT_WRITE;
	if (flags & UKPLAT_MEMRF_EXECUTE)
		prot |= PAGE_ATTR_PROT_EXEC;

	return prot;
}

static int paging_init(void)
{
	struct ukplat_memregion_desc *mrd;
	__sz len;
	__vaddr_t vaddr;
	__paddr_t paddr;
	unsigned long prot;
	int rc;

	/* Initialize the frame allocator with the free physical memory
	 * regions supplied via the boot info. The new page table uses the
	 * one currently active.
	 */
	rc = -ENOMEM; /* In case there is no region */
	ukplat_memregion_foreach(&mrd, UKPLAT_MEMRT_FREE, 0, 0) {
		paddr  = PAGE_ALIGN_UP(mrd->pbase);
		len    = PAGE_ALIGN_DOWN(mrd->len - (paddr - mrd->pbase));

		/* Not mapped */
		mrd->vbase = __U64_MAX;
		mrd->flags &= ~UKPLAT_MEMRF_PERMS;

		if (unlikely(len == 0))
			continue;

		if (!kernel_pt.fa) {
			rc = ukplat_pt_init(&kernel_pt, paddr, len);
			if (unlikely(rc))
				kernel_pt.fa = NULL;
		} else {
			rc = ukplat_pt_add_mem(&kernel_pt, paddr, len);
		}

		/* We do not fail if we cannot add this memory region to the
		 * frame allocator. If the range is too small to hold the
		 * metadata, this is expected. Just ignore this error.
		 */
		if (unlikely(rc && rc != -ENOMEM))
			uk_pr_err("Cannot add %12lx-%12lx to paging: %d\n",
				  paddr, paddr + len, rc);
	}

	if (unlikely(!kernel_pt.fa))
		return rc;

	/* Activate page table */
	rc = ukplat_pt_set_active(&kernel_pt);
	if (unlikely(rc))
		return rc;

	/* Perform unmappings */
	ukplat_memregion_foreach(&mrd, 0, UKPLAT_MEMRF_UNMAP,
				 UKPLAT_MEMRF_UNMAP) {
		UK_ASSERT(mrd->vbase != __U64_MAX);

		vaddr = PAGE_ALIGN_DOWN(mrd->vbase);
		len   = PAGE_ALIGN_UP(mrd->len + (mrd->vbase - vaddr));

		rc = ukplat_page_unmap(&kernel_pt, vaddr,
				       len >> PAGE_SHIFT,
				       PAGE_FLAG_KEEP_FRAMES);
		if (unlikely(rc))
			return rc;
	}

	/* Perform mappings */
	ukplat_memregion_foreach(&mrd, 0, UKPLAT_MEMRF_MAP,
				 UKPLAT_MEMRF_MAP) {
		UK_ASSERT(mrd->vbase != __U64_MAX);

		vaddr = PAGE_ALIGN_DOWN(mrd->vbase);
		paddr = PAGE_ALIGN_DOWN(mrd->pbase);
		len   = PAGE_ALIGN_UP(mrd->len + (mrd->vbase - vaddr));
		prot  = bootinfo_to_page_attr(mrd->flags);

		rc = ukplat_page_map(&kernel_pt, vaddr, paddr,
				     len >> PAGE_SHIFT, prot, 0);
		if (unlikely(rc))
			return rc;
	}

	return 0;
}

static int mem_init(struct ukplat_bootinfo *bi)
{
	int rc;

	/* TODO: Until we generate the boot page table at compile time, we
	 * manually add an untyped unmap region to the boot info to force an
	 * unmapping of the 1:1 mapping after the kernel image before mapping
	 * only the necessary parts.
	 */
	rc = ukplat_memregion_list_insert(&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = PAGE_ALIGN_UP(__END),
			.pbase = 0,
			.len   = PLATFORM_MAX_MEM_ADDR - PAGE_ALIGN_UP(__END),
			.type  = 0,
			.flags = UKPLAT_MEMRF_UNMAP,
		});
	if (unlikely(rc < 0))
		return rc;

	return paging_init();
}
#else /* CONFIG_HAVE_PAGING */
static int mem_init(struct ukplat_bootinfo *bi)
{
	struct ukplat_memregion_desc *mrdp;
	int i;

	/* The static boot page table maps only the first 4 GiB. Remove all
	 * free memory regions above this limit so we won't use them for the
	 * heap. Start from the tail as the memory list is ordered by address.
	 * We can stop at the first area that is completely in the mapped area.
	 */
	for (i = (int)bi->mrds.count - 1; i >= 0; i--) {
		ukplat_memregion_get(i, &mrdp);
		if (mrdp->vbase >= PLATFORM_MAX_MEM_ADDR) {
			/* Region is outside the mapped area */
			uk_pr_info("Memory %012lx-%012lx outside mapped area\n",
				   mrdp->vbase, mrdp->vbase + mrdp->len);

			if (mrdp->type == UKPLAT_MEMRT_FREE)
				ukplat_memregion_list_delete(&bi->mrds, i);
		} else if (mrdp->vbase + mrdp->len > PLATFORM_MAX_MEM_ADDR) {
			/* Region overlaps with unmapped area */
			uk_pr_info("Memory %012lx-%012lx outside mapped area\n",
				   PLATFORM_MAX_MEM_ADDR,
				   mrdp->vbase + mrdp->len);

			if (mrdp->type == UKPLAT_MEMRT_FREE)
				mrdp->len -= (mrdp->vbase + mrdp->len) -
						PLATFORM_MAX_MEM_ADDR;

			/* Since regions are non-overlapping and ordered, we
			 * can stop here, as the next region would be fully
			 * mapped anyways
			 */
			break;
		} else {
			/* Region is fully mapped */
			break;
		}
	}

	return 0;
}
#endif /* !CONFIG_HAVE_PAGING */

static char *cmdline;
static __sz cmdline_len;

static inline int cmdline_init(struct ukplat_bootinfo *bi)
{
	char *cmdl;

	if (bi->cmdline_len) {
		cmdl = (char *)bi->cmdline;
		cmdline_len = bi->cmdline_len;
	} else {
		cmdl = CONFIG_UK_NAME;
		cmdline_len = sizeof(CONFIG_UK_NAME) - 1;
	}

	/* This is not the original command-line, but one that will be thrashed
	 * by `ukplat_entry_argp` to obtain argc/argv. So mark it as a kernel
	 * resource instead.
	 */
	cmdline = ukplat_memregion_alloc(cmdline_len + 1, UKPLAT_MEMRT_KERNEL,
					 UKPLAT_MEMRF_READ |
					 UKPLAT_MEMRF_WRITE |
					 UKPLAT_MEMRF_MAP);
	if (unlikely(!cmdline))
		return -ENOMEM;

	memcpy(cmdline, cmdl, cmdline_len);
	cmdline[cmdline_len] = 0;

	return 0;
}

static void __noreturn _ukplat_entry2(void)
{
	/* It's not possible to unwind past this function, because the stack
	 * pointer was overwritten in lcpu_arch_jump_to. Therefore, mark the
	 * previous instruction pointer as undefined, so that debuggers or
	 * profilers stop unwinding here.
	 */
	ukarch_cfi_unwind_end();

	ukplat_entry_argp(NULL, cmdline, cmdline_len);

	ukplat_lcpu_halt();
}
#ifdef CONFIG_RUNTIME_ASLR
__u32 _gen_seed32(){
	__u32 low,high;
	__asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
	return ((unsigned long long)high << 32) | low;
}
static int uk_swrand_init(void)
{
	unsigned int i;
#ifdef CONFIG_LIBUKSWRAND_CHACHA
	unsigned int seedc = 10;
	__u32 seedv[10];
#else
	unsigned int seedc = 2;
	__u32 seedv[2];
#endif
	uk_pr_info("Initialize random number generator...\n");

	for (i = 0; i < seedc; i++)
		seedv[i] = _gen_seed32();

	uk_swrand_init_r(&uk_swrand_def, seedc, seedv);

	return seedc;
}
#endif
void _ukplat_entry(struct lcpu *lcpu, struct ukplat_bootinfo *bi)
{
	int rc;
	void *bstack;

	_libkvmplat_init_console();

	/* Initialize trap vector table */
	traps_table_init();

	/* Initialize LCPU of bootstrap processor */
	rc = lcpu_init(lcpu);
	if (unlikely(rc))
		UK_CRASH("Bootstrap processor init failed: %d\n", rc);

	/* Initialize IRQ controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Interrupt controller init failed: %d\n", rc);

	/* Initialize command line */
	rc = cmdline_init(bi);
	if (unlikely(rc))
		UK_CRASH("Cmdline init failed: %d\n", rc);

	#ifdef CONFIG_RUNTIME_ASLR
	uk_swrand_init();
	#endif

	/* Allocate boot stack */
	ukplat_bootinfo_print();
	#ifdef CONFIG_RUNTIME_ASLR
	bstack = stackmemory_aslr_palloc(__STACK_SIZE);
	#else
	bstack = bootmemory_palloc(__STACK_SIZE, UKPLAT_MEMRT_STACK);
	#endif
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");

	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/* Initialize memory */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Mem init failed: %d\n", rc);

	/* Print boot information */
	ukplat_bootinfo_print();

#if defined(CONFIG_HAVE_SMP) && defined(CONFIG_UKPLAT_ACPI)
	rc = acpi_init();
	if (likely(rc == 0)) {
		rc = lcpu_mp_init(CONFIG_UKPLAT_LCPU_RUN_IRQ,
				  CONFIG_UKPLAT_LCPU_WAKEUP_IRQ,
				  NULL);
		if (unlikely(rc))
			uk_pr_err("SMP init failed: %d\n", rc);
	} else {
		uk_pr_err("ACPI init failed: %d\n", rc);
	}
#endif /* CONFIG_HAVE_SMP && CONFIG_UKPLAT_ACPI */

#ifdef CONFIG_HAVE_SYSCALL
	_init_syscall();
#endif /* CONFIG_HAVE_SYSCALL */

#if CONFIG_HAVE_X86PKU
	_check_ospke();
#endif /* CONFIG_HAVE_X86PKU */

	/* Switch away from the bootstrap stack */
	uk_pr_info("Switch from bootstrap stack to stack @%p\n", bstack);
	lcpu_arch_jump_to(bstack, _ukplat_entry2);
}
