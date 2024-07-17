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

#include <uk/plat/lcpu.h>
#include <uk/plat/common/lcpu.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>

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
					 UKPLAT_MEMRF_WRITE);
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

	/* Allocate boot stack */
	bstack = ukplat_memregion_alloc(__STACK_SIZE, UKPLAT_MEMRT_STACK,
					UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE);
	if (unlikely(!bstack))
		UK_CRASH("Boot stack alloc failed\n");

	bstack = (void *)((__uptr)bstack + __STACK_SIZE);

	/* Initialize memory */
	rc = ukplat_mem_init();
	if (unlikely(rc))
		UK_CRASH("Mem init failed: %d\n", rc);

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
