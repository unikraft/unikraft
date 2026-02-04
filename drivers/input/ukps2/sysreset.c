/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/util.h>
#include <uk/essentials.h>
#include <uk/prio.h>

#include "ps2.h"

#include <uk/pm.h>

#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKBOOT */

__isr int uk_ps2_cpu_reset(void)
{
	/* Trigger the reset line via the PS/2 controller. On firecracker
	 * this will shutdown the VM.
	 */
	uk_arch_outb(PS2_CMD_REG, PS2_CMD_CPU_RESET);

	/* Return error if writing to port failed as that should not return. */
	return -EIO;
}

#if CONFIG_LIBUKPM
static const struct uk_pm_ops ps2_pm_ops = {
	.syshalt = uk_ps2_cpu_reset,
	.sysrestart = uk_ps2_cpu_reset,
	.syscrash = uk_ps2_cpu_reset,
};

#if CONFIG_LIBUKBOOT
__isr static int ps2_register_pm_ops(struct ukplat_bootinfo __unused *bi)
{
	return uk_pm_ops_register(&ps2_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(ps2_register_pm_ops, UK_PRIO_EARLIEST);
#endif /* CONFIG_LIBUKBOOT */
#endif /* CONFIG_LIBUKPM */
