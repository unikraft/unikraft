/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch.h>
#include <uk/arch/util.h>
#include <uk/boot/earlytab.h>
#include <uk/pm.h>
#include <uk/prio.h>

/* The port used by QEMU by default for the isa-debug-exit device */
#define QEMU_ISA_DEBUG_EXIT_PORT	0x501
/* This corresponds to an 83 (41 << 1 | 1) return value from QEMU */
#define QEMU_ISA_DEBUG_EXIT_NO_CRASH	41
/* This corresponds to an 85 (42 << 1 | 1) return value from QEMU */
#define QEMU_ISA_DEBUG_EXIT_CRASH	42

/* The port used by QEMU by default for the pvpanic device */
#define QEMU_PVPANIC_EXIT_PORT		0x505
/* This corresponds to a GUEST_PANICKED event for QEMU */
#define QEMU_PVPANIC_GUEST_PANICKED	(1 << 0)
/* This corresponds to a GUEST_CRASHLOADED event for QEMU */
#define QEMU_PVPANIC_GUEST_CRASHLOADED	(1 << 1)

/**
 * Trigger an exit() in QEMU with the code `value << 1 | 1`.
 * @param value the value used in the calculation of the exit code
 */
__isr static inline void qemu_debug_exit(int value)
{
	uk_arch_outw(QEMU_ISA_DEBUG_EXIT_PORT, value);
}

__isr static inline void _do_qemu_acpi_shutdown(void)
{
	/*
	 * Perform an ACPI shutdown by writing (SLP_TYPa | SLP_EN) to PM1a_CNT.
	 * Generally speaking, we'd have to jump through a lot of hoops to
	 * collect those values, however, for QEMU, those are static. Should be
	 * harmless if we're not running on QEMU, especially considering we're
	 * already shutting down, so who cares if we crash.
	 */
	uk_arch_outw(0x604, 0x2000);
}

__isr static int qemu_exit(void)
{
	qemu_debug_exit(QEMU_ISA_DEBUG_EXIT_NO_CRASH);

	/* Use QEMU hardcoded ACPI shutdown port/value as a fallback */
	_do_qemu_acpi_shutdown();

	/* Return error if writing to port failed as that should not return. */
	return -EIO;
}

__isr static int qemu_crash(void)
{
	/* If we are crashing, then try to exit QEMU with the isa-debug-exit
	 * device.
	 * Should be harmless if it is not present. This is used to enable
	 * automated tests on virtio.
	 * Also send a panic request to the pvpanic device.
	 * Should be also harmless and helps with automated tests.
	 */
	qemu_debug_exit(QEMU_ISA_DEBUG_EXIT_CRASH);
	uk_arch_outb(QEMU_PVPANIC_EXIT_PORT, QEMU_PVPANIC_GUEST_PANICKED);

	/* Use QEMU hardcoded ACPI shutdown port/value as a fallback */
	_do_qemu_acpi_shutdown();

	/* Return error if writing to port failed as that should not return. */
	return -EIO;
}

static const struct uk_pm_ops qemu_pm_ops = {
	.syshalt = qemu_exit,
	.sysrestart = qemu_exit,
	.syscrash = qemu_crash,
};

static int qemu_register_pm_ops(struct ukplat_bootinfo __unused *bi)
{
	return uk_pm_ops_register(&qemu_pm_ops);
}

UK_BOOT_EARLYTAB_ENTRY(qemu_register_pm_ops, UK_PRIO_EARLIEST);
