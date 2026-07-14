/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, Arm Ltd., All rights reserved.
 * Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <string.h>

#include <uk/arch/types.h>
#include <uk/assert.h>
#include <uk/compiler.h>
#include <uk/config.h>
#include <uk/lcpu.h>
#include <uk/paging.h>

#if CONFIG_LIBUKACPI
#include <uk/acpi.h>
#endif /* CONFIG_LIBUKACPI */

#if CONFIG_LIBUKOFW
#include <uk/ofw/fdt.h>
#endif /* CONFIG_LIBUKOFW */
#include <uk/pcpuvar.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/prio.h>
#include <uk/print.h>
#include <uk/psci.h>
#include <uk/smccc.h>

#if CONFIG_LIBUKBOOT && CONFIG_LIBUKPM
#include <uk/boot/earlytab.h>
#include <uk/pm.h>
#endif /* CONFIG_LIBUKBOOT && CONFIG_LIBUKPM */

/*
 * PSCI function codes (as per PSCI v0.2).
 */
#define PSCI_FNID_VERSION               0x84000000
#define PSCI_FNID_CPU_SUSPEND           0xc4000001
#define PSCI_FNID_CPU_OFF               0x84000002
#define PSCI_FNID_CPU_ON                0xc4000003
#define PSCI_FNID_AFFINITY_INFO         0xc4000004
#define PSCI_FNID_MIGRATE               0xc4000005
#define PSCI_FNID_MIGRATE_INFO_TYPE     0x84000006
#define PSCI_FNID_MIGRATE_INFO_UP_CPU   0xc4000007
#define PSCI_FNID_SYSTEM_OFF            0x84000008
#define PSCI_FNID_SYSTEM_RESET          0x84000009
#define PSCI_FNID_FEATURES              0x8400000a

static uk_smccc_conduit_func smccc_psci_call;

/* Systems support PSCI >= 0.2 can do system reset from PSCI */
__isr int uk_psci_reset(void)
{
	struct uk_smccc_args smccc_arguments = {0};

	/*
	 * NO PSCI or invalid PSCI method, we can't do reset, just
	 * halt the CPU.
	 */
	if (!smccc_psci_call) {
		uk_pr_crit("Couldn't reset system, HALT!\n");
		uk_lcpu_halt();
	}

	smccc_arguments.a0 = PSCI_FNID_SYSTEM_RESET;
	smccc_psci_call(&smccc_arguments);

	/* The PSCI call should not return, and if it does
	 * we cannot do call UK_BUG or UK_CRASH as they would
	 * bring us back here. Return an error code, and let
	 * the caller handle the error.
	 */
	return -EIO;
}

/* Systems support PSCI >= 0.2 can do system off from PSCI */
__isr int uk_psci_system_off(void)
{
	struct uk_smccc_args smccc_arguments = {0};

	/*
	 * NO PSCI or invalid PSCI method, we can't do shutdown, just
	 * halt the CPU.
	 */
	if (!smccc_psci_call) {
		uk_pr_crit("Couldn't shutdown system, HALT!\n");
		uk_lcpu_halt();
	}

	smccc_arguments.a0 = PSCI_FNID_SYSTEM_OFF;
	smccc_psci_call(&smccc_arguments);

	/* The PSCI call should not return, and if it does
	 * we cannot do call UK_BUG or UK_CRASH as they would
	 * bring us back here. Return an error code, and let
	 * the caller handle the error.
	 */
	return -EIO;
}

/* Powers on a secondary cpu in an SMP system. */
__isr int uk_psci_cpu_on(__u64 cpuid, __u64 entry_point, __u64 context_id)
{
	struct uk_smccc_args smccc_arguments = {0};

	/*
	 * Check if a PSCI method is set.
	 */
	UK_ASSERT(smccc_psci_call);

	smccc_arguments.a0 = PSCI_FNID_CPU_ON;
	smccc_arguments.a1 = cpuid;
	smccc_arguments.a2 = entry_point;
	smccc_arguments.a3 = context_id;

	smccc_psci_call(&smccc_arguments);

	return (int)smccc_arguments.a0;
}

#if CONFIG_LIBUKACPI
int uk_psci_init(struct ukplat_bootinfo *bi __unused)
{
	struct uk_acpi_fadt *acpi_fadt = uk_acpi_get_fadt();

	if (unlikely(!acpi_fadt))
		return -ENOTSUP;

	if (unlikely(!(acpi_fadt->arm_bflags & UK_ACPI_FADT_ARM_BFLAGS_PSCI)))
		return -ENOTSUP;

	if (acpi_fadt->arm_bflags & UK_ACPI_FADT_ARM_BFLAGS_PSCI_HVC)
		smccc_psci_call = uk_smccc_hvc;
	else
		smccc_psci_call = uk_smccc_smc;

	uk_pr_info("PSCI method: %s\n",
		   (smccc_psci_call == uk_smccc_hvc)   ? "hvc"
		   : (smccc_psci_call == uk_smccc_smc) ? "smc"
						       : "unknown");

	return 0;
}
#else /* !CONFIG_LIBUKACPI */
int uk_psci_init(struct ukplat_bootinfo *bi)
{
	const char *fdtmethod;
	int fdtpsci, len;
	void *fdt;

	fdt = (void *)bi->dtb;
	UK_ASSERT(bi->dtb);

	/*
	 * We just support PSCI-0.2 and PSCI-1.0, the PSCI-0.1 would not
	 * be supported.
	 */
	fdtpsci = fdt_node_offset_by_compatible(fdt, -1, "arm,psci-1.0");
	if (unlikely(fdtpsci < 0))
		fdtpsci = fdt_node_offset_by_compatible(fdt, -1,
							"arm,psci-0.2");

	if (unlikely(fdtpsci < 0)) {
		uk_pr_info("No PSCI conduit found in DTB\n");
		return -ENOENT;
	}

	fdtmethod = fdt_getprop(fdt, fdtpsci, "method", &len);
	if (unlikely(!fdtmethod || len <= 0)) {
		uk_pr_info("No PSCI method found\n");
		return -ENOENT;
	}

	if (!strcmp(fdtmethod, "hvc")) {
		smccc_psci_call = uk_smccc_hvc;
	} else if (!strcmp(fdtmethod, "smc")) {
		smccc_psci_call = uk_smccc_smc;
	} else {
		uk_pr_info("Invalid PSCI conduit method: %s\n", fdtmethod);
		return -EINVAL;
	}
	uk_pr_info("PSCI method: %s\n", fdtmethod);

	return 0;
}
#endif /* !CONFIG_LIBUKACPI */

#if CONFIG_LIBUKBOOT && CONFIG_LIBUKPM
static const struct uk_pm_ops psci_pm_ops = {
	.syshalt = uk_psci_system_off,
	.sysrestart = uk_psci_reset,
	.syscrash = uk_psci_system_off,
};

static int psci_register_pm_ops(struct ukplat_bootinfo *bi)
{
	int rc;

	rc = uk_psci_init(bi);
	if (unlikely(rc))
		return rc;

	return uk_pm_ops_register(&psci_pm_ops);
}

/* Priority must be later than ACPI */
UK_BOOT_EARLYTAB_ENTRY(psci_register_pm_ops, UK_PRIO_AFTER(UK_PRIO_EARLIEST));
#endif /* CONFIG_LIBUKBOOT && CONFIG_LIBUKPM */
