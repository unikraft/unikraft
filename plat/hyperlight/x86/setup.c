/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Platform setup for Hyperlight.
 *
 * hyperlight_entry() is the first C function called (from lcpu_start.S).
 * It initialises the console, parses the PEB to register memory, and
 * hands off to _ukplat_entry() for standard Unikraft boot.
 *
 * _ukplat_entry() follows the same pattern as plat/kvm: LCPU init,
 * early init, IRQ controller probe, memory init, stack allocation,
 * then jump to uk_boot_entry().
 */

#include <string.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/arch/x86_64.h>
#include <uk/asm/cfi.h>
#include <uk/boot.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/intctlr.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/plat/memory.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>

#include <hyperlight-x86/dispatch.h>
#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/peb.h>
#include <hyperlight-x86/setup.h>

/* Console init provided by console.c */
void _ukplat_init_console(void);

/* Entry args saved by entry64.S into .data */
extern struct hyperlight_entry_args hyperlight_entry_args;

/* Global PEB pointer — other modules may need it later */
static struct hyperlight_peb *g_peb;

/* Command line buffer — populated from the host via hcall at boot.
 * Falls back to a default if the host doesn't provide GetCmdLine.
 */
static char hyperlight_cmdline[4096] = "unikraft-hyperlight";

/**
 * Register PEB memory regions in bootinfo.
 *
 * The PEB's guest_heap region is registered as FREE memory so that
 * Unikraft's allocator can use it.  The input/output stacks are for
 * the FlatBuffer-based host-call protocol and are not general memory.
 */
static void hyperlight_init_mem(struct ukplat_bootinfo *bi,
				struct hyperlight_peb *peb)
{
	struct ukplat_memregion_desc mrd = {0};
	int rc;

	if (peb->guest_heap.size > 0) {
		mrd.pbase = peb->guest_heap.ptr;
		mrd.vbase = mrd.pbase; /* identity-mapped */
		mrd.pg_off = 0;
		mrd.len = peb->guest_heap.size;
		mrd.pg_count = mrd.len / __PAGE_SIZE;
		mrd.type = UKPLAT_MEMRT_FREE;
		mrd.flags = UKPLAT_MEMRF_READ | UKPLAT_MEMRF_WRITE;
#ifdef CONFIG_UKPLAT_MEMRNAME
		memcpy(mrd.name, "heap", sizeof("heap"));
#endif
		rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
		if (unlikely(rc < 0))
			UK_CRASH("Unable to add heap region\n");
	}

}

static void __noreturn ukplat_entry2(void *arg __unused)
{
	ukarch_cfi_unwind_end();
	uk_boot_entry();
	UK_BUG(); /* noreturn */
}

static void _ukplat_entry(struct ukplat_bootinfo *bi)
{
	void *bstack;
	int rc;

	/*
	 * Pre-fault the native PAL's IST exception stacks before
	 * uk_lcpu_init() replaces the IDT.  The stacks live in BSS
	 * (CoW); if we don't fault the pages in now, the first page
	 * fault after lidt would try to push onto a read-only IST
	 * stack → double fault → triple fault.
	 */
	{
		volatile __u8 *p;
		/* 3 IST stacks, each CPU_EXCEPT_STACK_SIZE (PAGE_SIZE << order) */
		__sz total = 3UL * (__PAGE_SIZE <<
				    CONFIG_CPU_EXCEPT_STACK_SIZE_PAGE_ORDER);
		__sz i;

		p = (volatile __u8 *)uk_pal_except_get_except_stack_base();
		for (i = 0; i < total; i += __PAGE_SIZE) {
			__u8 tmp = *(p + i);
			*(volatile __u8 *)(p + i) = tmp;
		}
	}

	/* Initialize LCPU of bootstrap processor.
	 * This installs the native PAL's full IDT, replacing the
	 * minimal 1-entry IDT from entry64.S.
	 */
	rc = uk_lcpu_init(uk_pcpuvar_current_ptr_get(uk_lcpus));
	if (unlikely(rc))
		UK_CRASH("Bootstrap processor init failed: %d\n", rc);

	/* Switch from asm CoW handler to C event-based handler.
	 * Must be after uk_lcpu_init() since page faults now go
	 * through the native PAL's event system.
	 */
	{
		extern void hyperlight_cow_init(void);

		hyperlight_cow_init();
	}

#ifdef CONFIG_HAVE_SYSCALL
	/*
	 * Program SYSCALL MSRs so ring-3 apps (e.g. via elfloader's
	 * execve) can transition to ring-0 via the syscall instruction.
	 * Same setup as plat/kvm/x86/setup.c, minus EFER.
	 *
	 * EFER is deliberately NOT touched here.  Hyperlight already
	 * enters the guest with EFER = LME|LMA|SCE|NX (it is part of the
	 * host-programmed special-register state), so SCE is on before we
	 * run.  From hyperlight 0.17.0 the vCPU also runs behind a
	 * default-deny KVM MSR filter that only permits the MSRs the host
	 * declares via SandboxConfiguration::guest_msrs(); EFER cannot be
	 * declared (it is sregs state, not a resettable MSR), so a guest
	 * rdmsr/wrmsr of it faults with #GP.  STAR/LSTAR/SYSCALL_MASK are
	 * declared by the host and remain writable here.
	 */
	{
		extern void _ukplat_syscall(void);

		uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_STAR,
			(0x08ULL << 48) | (0x08ULL << 32));
		uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_LSTAR,
			(__uptr)_ukplat_syscall);
		uk_arch_x86_64_wrmsrl(UK_ARCH_X86_64_MSR_SYSCALL_MASK,
			UK_ARCH_X86_64_RFLAGS_TF |
			UK_ARCH_X86_64_RFLAGS_DF |
			UK_ARCH_X86_64_RFLAGS_IF |
			UK_ARCH_X86_64_RFLAGS_AC |
			UK_ARCH_X86_64_RFLAGS_NT);
	}
#endif /* CONFIG_HAVE_SYSCALL */

	/* Execute early init */
	uk_boot_early_init(bi);

	/* Initialize IRQ controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Interrupt controller init failed: %d\n", rc);

	/* Allocate boot stack before ukplat_mem_init() — paging init
	 * hands free regions to the frame allocator and clears their
	 * permission flags, making them invisible to memregion_alloc.
	 */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");

	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/*
	 * Pre-fault the new boot stack for CoW before switching.
	 * The allocator returns heap memory from the PEB, which
	 * Hyperlight maps as CoW.  Without pre-faulting, the first
	 * push after switching RSP faults on a read-only page.
	 */
	{
		volatile __u8 *p = (volatile __u8 *)bstack;
		__sz i;

		for (i = __PAGE_SIZE; i <= __STACK_SIZE; i += __PAGE_SIZE) {
			__u8 tmp = *(p - i);
			*(volatile __u8 *)(p - i) = tmp;
		}
	}

	/* Initialize memory — after boot stack is allocated, since
	 * paging init consumes free regions for the frame allocator.
	 */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Mem init failed: %d\n", rc);

	/* Switch away from the bootstrap stack */
	uk_arch_x86_64_jump_to((__u64)bstack, (__u64)ukplat_entry2);
}

/**
 * C entry point called from lcpu_start.S.
 *
 * @param lcpu       Pointer to bootstrap LCPU structure
 * @param entry_args Pointer to hyperlight_entry_args in .data
 */
void hyperlight_entry(struct uk_lcpu *lcpu __unused,
		      struct hyperlight_entry_args *entry_args)
{
	struct ukplat_bootinfo *bi;

	/* Initialize console so uk_pr_* works */
	_ukplat_init_console();

	/* Store PEB globally */
	g_peb = (struct hyperlight_peb *)entry_args->peb_address;
	if (unlikely(!g_peb))
		UK_CRASH("PEB address is NULL\n");

	/* Get bootinfo (pre-populated by linker via bootinfo.lds.S) */
	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Incompatible or corrupted bootinfo\n");

	/* Register PEB memory regions */
	hyperlight_init_mem(bi, g_peb);

	/* Initialise the host-call subsystem so we can query the host */
	hl_hcall_init(g_peb);

	/* Initialise the dispatch subsystem with a copy of the PEB.
	 * The dispatch handler reads/writes the PEB I/O stacks when
	 * the host calls guest functions after evolve().
	 */
	hyperlight_dispatch_init(g_peb);

	/* Try to get the command line from the host via hcall.
	 * If the host doesn't register GetCmdLine, the default
	 * "unikraft-hyperlight" stays in the buffer.
	 */
	{
		int len = hl_call_get_cmdline(hyperlight_cmdline,
					      sizeof(hyperlight_cmdline));
		if (len < 0)
			uk_pr_info("GetCmdLine: using default cmdline\n");
		else
			uk_pr_info("GetCmdLine: \"%s\"\n", hyperlight_cmdline);
	}

	/* Query the host for a mapped initrd and register it.
	 * The host decides where to map the initrd (via map_file_cow)
	 * and tells us via GetInitrdBase/GetInitrdSize.
	 */
	{
		__u64 initrd_base = hl_call_get_initrd_base();
		__u64 initrd_size = hl_call_get_initrd_size();

		/*
			 * TODO: consider crashing when exactly one of
			 * base/size is zero — that likely indicates a
			 * misconfigured host rather than "no initrd".
			 */
			if (initrd_base > 0 && initrd_size > 0) {
			struct ukplat_memregion_desc mrd = {0};
			int rc;

			mrd.pbase = initrd_base;
			mrd.vbase = initrd_base; /* identity-mapped */
			mrd.pg_off = 0;
			mrd.len = initrd_size;
			mrd.pg_count = (initrd_size + __PAGE_SIZE - 1) / __PAGE_SIZE;
			mrd.type = UKPLAT_MEMRT_INITRD;
			mrd.flags = UKPLAT_MEMRF_READ;
#ifdef CONFIG_UKPLAT_MEMRNAME
			memcpy(mrd.name, "initrd", sizeof("initrd"));
#endif
			rc = ukplat_memregion_list_insert(&bi->mrds, &mrd);
			if (unlikely(rc < 0))
				UK_CRASH("Unable to add initrd region\n");

			uk_pr_info("Initrd: %lu bytes at GPA %lx\n",
				   (unsigned long)initrd_size,
				   (unsigned long)initrd_base);
		}
	}

	/* Set boot protocol and command line */
	memcpy(bi->bootprotocol, "hyperlight", sizeof("hyperlight"));
	bi->cmdline = (__u64)hyperlight_cmdline;
	bi->cmdline_len = strlen(hyperlight_cmdline);

	_ukplat_entry(bi);
}
