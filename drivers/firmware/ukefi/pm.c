/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/efi.h>
#include <uk/essentials.h>
#include <uk/prio.h>

#include <uk/pm.h>
#if CONFIG_LIBUKBOOT
#include <uk/boot/earlytab.h>
#endif /* CONFIG_LIBUKBOOT */

static const struct uk_efi_runtime_services *rs;

/**
 * Halt the system via the UEFI Runtime Service ResetSystem().
 *
 * Issues a shutdown reset by invoking reset_system() with
 * UK_EFI_RESET_SHUTDOWN, which instructs the firmware to power off
 * the platform. The reason string "UK EFI SYSTEM SHUTDOWN" is
 * forwarded to the firmware as the reset data.
 */
__isr int uk_efi_rs_syshalt(void)
{
	UK_ASSERT(rs);

	rs->reset_system(UK_EFI_RESET_SHUTDOWN, UK_EFI_SUCCESS,
			 sizeof("UK EFI SYSTEM SHUTDOWN"),
			 "UK EFI SYSTEM SHUTDOWN");

	/* Return error if the call failed as that should not return. */
	return -EIO;
}

/**
 * Restart the system via the UEFI Runtime Service ResetSystem().
 *
 * Issues a cold reset by invoking reset_system() with
 * UK_EFI_RESET_COLD, which causes the firmware to fully reinitialize
 * all platform hardware from a powered-off state. The reason string
 * "UK EFI SYSTEM RESET" is forwarded to the firmware as the reset data.
 */
__isr int uk_efi_rs_sysrestart(void)
{
	UK_ASSERT(rs);

	rs->reset_system(UK_EFI_RESET_COLD, UK_EFI_SUCCESS,
			 sizeof("UK EFI SYSTEM RESET"), "UK EFI SYSTEM RESET");

	/* Return error if the call failed as that should not return. */
	return -EIO;
}

/**
 * Halt the system via the UEFI Runtime Service ResetSystem(),
 * signalling that the shutdown was caused by a crash.
 *
 * Issues a shutdown reset by invoking reset_system() with
 * UK_EFI_RESET_SHUTDOWN. The reason string "UK EFI SYSTEM CRASH" is
 * forwarded to the firmware as the reset data to distinguish this from
 * a normal halt.
 */
__isr int uk_efi_rs_syscrash(void)
{
	UK_ASSERT(rs);

	rs->reset_system(UK_EFI_RESET_SHUTDOWN, UK_EFI_SUCCESS,
			 sizeof("UK EFI SYSTEM CRASH"), "UK EFI SYSTEM CRASH");

	/* Return error if the call failed as that should not return. */
	return -EIO;
}

int uk_efi_rs_init(const struct uk_efi_sys_tbl *st)
{
	UK_ASSERT(st);

	rs = st->runtime_services;
	if (unlikely(!rs))
		return -ENODEV;

	return 0;
}

#if CONFIG_LIBUKPM
static const struct uk_pm_ops efi_rs_pm_ops = {
	.syshalt = uk_efi_rs_syshalt,
	.sysrestart = uk_efi_rs_sysrestart,
	.syscrash = uk_efi_rs_syscrash,
};

#if CONFIG_LIBUKBOOT
__isr static int efi_rs_register_pm_ops(struct ukplat_bootinfo *bi)
{
	const struct uk_efi_sys_tbl *st;
	int rc;

	UK_ASSERT(bi);

	st = (struct uk_efi_sys_tbl *)bi->efi_st;
	if (unlikely(!st))
		return -ENODEV;

	rc = uk_efi_rs_init(st);
	if (unlikely(rc))
		return -ENODEV;

	return uk_pm_ops_register(&efi_rs_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(efi_rs_register_pm_ops, UK_PRIO_EARLIEST);
#endif /* CONFIG_LIBUKBOOT */
#endif /* CONFIG_LIBUKPM */
