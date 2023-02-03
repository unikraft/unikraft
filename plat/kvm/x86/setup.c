/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>
#include <x86/cpu.h>
#include <x86/traps.h>
#include <x86/acpi/acpi.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/arch/paging.h>
#include <uk/plat/console.h>
#include <uk/assert.h>
#include <uk/essentials.h>

#include <kvm/console.h>
#include <kvm/intctrl.h>

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
	char *cmdl = (bi->cmdline) ? (char *)bi->cmdline : CONFIG_UK_NAME;

	cmdline_len = strlen(cmdl) + 1;

	cmdline = bootmemory_palloc(cmdline_len, UKPLAT_MEMRT_CMDLINE);
	if (unlikely(!cmdline))
		return -ENOMEM;

	strncpy(cmdline, cmdl, cmdline_len);
	return 0;
}

static void __noreturn _ukplat_entry2(void)
{
	ukplat_entry_argp(NULL, cmdline, cmdline_len);

	ukplat_lcpu_halt();
}

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
	intctrl_init();

	/* Initialize command line */
	rc = cmdline_init(bi);
	if (unlikely(rc))
		UK_CRASH("Cmdline init failed: %d\n", rc);

	/* Allocate boot stack */
	bstack = bootmemory_palloc(__STACK_SIZE, UKPLAT_MEMRT_STACK);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");

	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/* Initialize memory */
	rc = mem_init(bi);
	if (unlikely(rc))
		UK_CRASH("Mem init failed: %d\n", rc);

	/* Print boot information */
	ukplat_bootinfo_print();

#ifdef CONFIG_HAVE_SMP
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
#endif /* CONFIG_HAVE_SMP */

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
