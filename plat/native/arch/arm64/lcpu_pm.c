/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2022, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/arm64.h>
#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/arch/ctx.h>
#include <uk/assert.h>
#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKBOOT */
#include <uk/compiler.h>
#include <uk/lcpu/pm.h>
#include <uk/pcpuvar.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/plat/config.h>
#include <uk/prio.h>
#include <uk/paging.h>
#include <uk/psci.h>

#if CONFIG_HAVE_SMP
extern void lcpu_start(void);

static int plat_native_lcpu_start(__u64 idx)
{
	__u64 cpuid;
	__u64 entry;

	cpuid = uk_pcpuvar_lval(idx, uk_pcpuvar_cpu_id);
	entry = uk_paging_virt_to_phys((__vaddr_t)lcpu_start);

	return uk_psci_cpu_on(cpuid, entry, idx);
}
#endif /* CONFIG_HAVE_SMP */

static void plat_native_lcpu_halt(void)
{
	uk_arch_arm64_halt();
}

static void plat_native_lcpu_halt_irq(void)
{
	UK_ASSERT(uk_plat_native_irqs_disabled());

	/* Note: If priority masking is enabled
	 * interrupts need to be unmasked in the GIC.
	 *
	 * See Linux `cpu_do_idle(void)` implementation
	 */
	uk_arch_arm64_halt();
}

static const struct uk_lcpu_pm_ops plat_native_pm_ops = {
#if CONFIG_HAVE_SMP
	.start = plat_native_lcpu_start,
#endif /* CONFIG_HAVE_SMP */
	.halt = plat_native_lcpu_halt,
	.halt_irq = plat_native_lcpu_halt_irq,
};

#if CONFIG_LIBUKBOOT
__isr static int
plat_native_lcpu_pm_ops_register(struct ukplat_bootinfo *bi __unused)
{
	return uk_lcpu_pm_ops_register(&plat_native_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(plat_native_lcpu_pm_ops_register, UK_PRIO_EARLIEST);
#endif /* CONFIG_LIBUKBOOT */
