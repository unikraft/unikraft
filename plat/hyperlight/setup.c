/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Platform setup for Hyperlight.
 *
 * hyperlight_entry() is the first C function called (x86: from
 * lcpu_start.S; arm64: from entry64.S).
 * It initialises the console, registers the initrd the host maps, and
 * hands off to _ukplat_entry() for standard Unikraft boot.
 *
 * _ukplat_entry() follows the same pattern as plat/kvm: LCPU init,
 * early init, IRQ controller probe, memory init, boot stack switch,
 * then jump to uk_boot_entry().
 */

#include <string.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#if defined(__x86_64__)
#include <uk/arch/x86_64.h>
#elif defined(__aarch64__)
#include <uk/arch/arm64.h>
#include <uk/plat/config.h>
#endif
#if defined(__x86_64__)
#include <uk/asm/cfi.h>
#endif
#include <uk/boot.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/intctlr.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/plat/memory.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>

#include <hyperlight/console.h>
#include <hyperlight/dispatch.h>
#include <hyperlight/hcall.h>
#include <hyperlight/peb.h>	/* only for its layout assertion */
#include <hyperlight/setup.h>

/* Console init provided by console.c */
void _ukplat_init_console(void);

/* Entry args saved by entry64.S into .data */
extern struct hyperlight_entry_args hyperlight_entry_args;

/* Command line buffer — populated from the host via hcall at boot.
 * Falls back to a default if the host doesn't provide GetCmdLine.
 */
static char hyperlight_cmdline[4096] = "unikraft-hyperlight";

/* Boot stack.  It becomes the "init" thread's: it runs the init table,
 * then waits on it for shutdown (or runs main() on it, without
 * LIBUKBOOT_MAINTHREAD).
 *
 * 1 MiB leaves room for the init table and a native main().  It is not
 * __STACK_SIZE, so that raising CONFIG_STACK_SIZE_PAGE_ORDER for the
 * threads that run guest code does not grow the stack copied into
 * scratch at every boot.
 *
 * It lives in .bss.  This kernel takes no memory from a host heap region
 * (it reads nothing from the PEB), and the kernel heap comes up only later
 * in boot.  Memory taken straight from scratch (hl_scratch_alloc_pages(),
 * used through the scratch window) is not captured by a snapshot, and a
 * restore zeroes it; .bss is snapshot memory, captured with the init
 * thread parked on it.
 */
#define HYPERLIGHT_BOOT_STACK_SIZE ((__sz)__PAGE_SIZE << 8)
static __u8 hyperlight_boot_stack[HYPERLIGHT_BOOT_STACK_SIZE]
	__align(__PAGE_SIZE);

/* Written to the boot stack's lowest word before the boot runs on it.
 * Code that runs the stack that far overwrites it, and may go on into the
 * .bss below.  A guard page would need the stack in a section of its own,
 * with the page below it unmapped and kept unmapped across snapshot and
 * restore; the canary needs neither.  A frame that skips the word goes
 * unnoticed.
 *
 * Checked at four points.  The halt that ends boot
 * (hyperlight_halt_boot_done()) catches the boot: an overflow during it
 * fails the boot.  Each later entry from the host
 * (hyperlight_dispatch_inner()), every guest function call and the
 * checkpoint before a snapshot, catches later use of the stack, by the
 * init thread (a native main() without LIBUKBOOT_MAINTHREAD that outlives
 * the boot halt) or by a stray write, so a guest whose canary is gone
 * fails the capture rather than being snapshotted.  The end of a call's
 * entry, before its result, catches the call's own work, so a call that
 * ran the stack down fails rather than returning.  A shutdown
 * (hyperlight_shutdown(), shutdown.c), usually run by the init thread, is
 * checked before it reports the exit, so a call in which the stack was run
 * down and the guest then shut down fails rather than ending cleanly.  A
 * snapshot of a guest with no call since its last snapshot or restore
 * enters no guest code: it captures memory the boot halt or a checkpoint
 * has already checked.  A host entry runs on the scratch exception stack
 * while the init thread is parked on this one.  The callers crash through
 * hyperlight_boot_stack_crash() (shutdown.c), not UK_CRASH, so the crash
 * cannot re-enter the shutdown's check; each knows whether the host takes
 * a host call there, and so how the report can reach it.
 */
#define HYPERLIGHT_BOOT_STACK_CANARY 0x216b7473746f6f62ULL /* "bootstk!" */

int hyperlight_boot_stack_intact(void)
{
	return *(volatile __u64 *)hyperlight_boot_stack ==
	       HYPERLIGHT_BOOT_STACK_CANARY;
}

static void __noreturn ukplat_entry2(void *arg __unused)
{
#if defined(__x86_64__)
	ukarch_cfi_unwind_end();
#endif
	uk_boot_entry();
	UK_BUG(); /* noreturn */
}

static void _ukplat_entry(struct ukplat_bootinfo *bi)
{
	void *bstack;
	int rc;

	/*
	 * Pre-fault the native PAL's exception stacks before its
	 * exception handling takes over.  The stacks live in BSS (CoW);
	 * if we don't fault the pages in now, the first page fault
	 * after would try to save its frame onto a read-only stack.
	 * On x86 that ends in a triple fault (#PF → #DF on read-only
	 * IST stacks); on arm64 the vectors fault on their own stores
	 * until the stack overflows.
	 */
	{
		volatile __u8 *p;
		/* 3 exception stacks (IST on x86), each PAGE_SIZE << order */
		__sz total = 3UL * (__PAGE_SIZE <<
				    CONFIG_CPU_EXCEPT_STACK_SIZE_PAGE_ORDER);
		__sz i;

		p = (volatile __u8 *)uk_pal_except_get_except_stack_base();
		for (i = 0; i < total; i += __PAGE_SIZE) {
			__u8 tmp = *(p + i);
			*(volatile __u8 *)(p + i) = tmp;
		}
	}

	/* Ready the C CoW handler, which takes over from the early asm one
	 * once the native PAL's vectors are installed below.  Before that:
	 * its own writes to .bss may fault, and only the early handler, still
	 * in place here, can serve them.
	 */
	{
		extern void hyperlight_cow_init(void);

		hyperlight_cow_init();
	}

	/* Initialize LCPU of bootstrap processor.
	 * On x86 this installs the native PAL's full IDT, replacing the
	 * minimal 1-entry IDT from entry64.S.
	 */
	rc = uk_lcpu_init(uk_pcpuvar_current_ptr_get(uk_lcpus));
	if (unlikely(rc))
		UK_CRASH("Bootstrap processor init failed: %d\n", rc);

#if defined(__aarch64__)
	/* On arm64 the platform owns VBAR_EL1: move from the early CoW
	 * vectors (entry64.S) to the native PAL's, now that uk_lcpu_init()
	 * has told them where their exception stacks are.
	 */
	{
		extern char vector_table[];

		UK_ARCH_ARM64_SYSREG_WRITE64(VBAR_EL1, (__u64)vector_table);
		uk_arch_arm64_isb();
	}
#endif /* __aarch64__ */

#if defined(__x86_64__) && defined(CONFIG_HAVE_SYSCALL)
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
#endif /* __x86_64__ && CONFIG_HAVE_SYSCALL */

	/* Execute early init */
	uk_boot_early_init(bi);

	/* Initialize IRQ controller */
	rc = uk_intctlr_probe();
	if (unlikely(rc))
		UK_CRASH("Interrupt controller init failed: %d\n", rc);

	bstack = hyperlight_boot_stack + HYPERLIGHT_BOOT_STACK_SIZE;

	/*
	 * Pre-fault the new boot stack before switching to it: .bss is
	 * snapshot memory, which Hyperlight maps CoW.  The C CoW handler,
	 * installed by now and running on the exception stacks on both
	 * architectures, would also take these faults as the stack grows.
	 * Faulting every page here makes the copies at one known point,
	 * before the frame allocator takes its share of scratch, rather
	 * than scattered over the rest of boot.
	 */
	{
		volatile __u8 *p = (volatile __u8 *)bstack;
		__sz i;

		for (i = __PAGE_SIZE; i <= HYPERLIGHT_BOOT_STACK_SIZE;
		     i += __PAGE_SIZE) {
			__u8 tmp = *(p - i);
			*(volatile __u8 *)(p - i) = tmp;
		}
	}
	*(volatile __u64 *)hyperlight_boot_stack = HYPERLIGHT_BOOT_STACK_CANARY;

	/* Initialize memory: the frame allocator, from scratch */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Mem init failed: %d\n", rc);

	/* Switch away from the bootstrap stack */
	uk_arch_jump_to((__u64)bstack, (__u64)ukplat_entry2);
}

/**
 * C entry point called from lcpu_start.S (x86) or entry64.S (arm64).
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
	hyperlight_console_log_init(entry_args->max_log_level);

	/* Nothing is read from the PEB (see hyperlight/peb.h), but every
	 * Hyperlight host passes one: a host that passes none is not one this
	 * kernel was built for.
	 */
	if (unlikely(!entry_args->peb_address))
		UK_CRASH("PEB address is NULL\n");

	/* Get bootinfo (pre-populated by linker via bootinfo.lds.S) */
	bi = ukplat_bootinfo_get();
	if (unlikely(!bi))
		UK_CRASH("Incompatible or corrupted bootinfo\n");

	/* Attach to the virtqueue transport so we can query the host */
	hl_hcall_init();

	/* Initialise the dispatch subsystem, which serves the guest
	 * function calls the host makes after evolve().
	 */
	hyperlight_dispatch_init();

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
