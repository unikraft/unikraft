/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_PLAT_NATIVE_ARCH_LCPU_H__
#define __UK_PLAT_NATIVE_ARCH_LCPU_H__

#include <uk/arch/types.h>
#include <uk/arch/util.h>
#include <uk/config.h>

#if !__ASSEMBLY__
#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBUKPLAT_NATIVE_LCPU
extern __vaddr_t uk_plat_native_x86_64_start16_addr; /* target address */

/**
 * Get x86_64 logical CPU hardware ID (APIC ID).
 * Reads the initial APIC ID from CPUID leaf 1 (EBX[31:24]).
 *
 * @return APIC ID of the current logical CPU
 */
static inline __u64 uk_plat_native_lcpu_id(void)
{
	__u32 eax, ebx, ecx, edx;

	uk_arch_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	return (ebx >> 24);
}

/**
 * Get logical CPU index (sequential 0..N-1 identifier).
 * Reads the index from a fixed offset in the GS_BASE-relative LCPU structure.
 * This is faster than APIC ID lookup and used for per-CPU data access.
 *
 * @return Sequential index of the current logical CPU
 */
static inline __u32 uk_plat_native_lcpu_idx(void)
{
	return uk_arch_rdgsbase32(UK_LCPU_IDX_OFFSET);
}

/**
 * Initialize x86_64 logical CPU structures.
 * Sets up per-CPU state including GS_BASE register for fast per-CPU access.
 * Must be called once per CPU during boot.
 *
 * @param this_lcpu  Pointer to LCPU structure for this CPU
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_init(struct uk_lcpu *this_lcpu);

#if CONFIG_HAVE_SMP
/**
 * Detect and populate LCPU structures for all x86_64 CPUs.
 * Enumerates logical CPUs via ACPI MADT or MP tables and creates
 * corresponding LCPU structures. Called once during boot on BSP.
 *
 * @param arg  Reserved for future use (currently unused)
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_mp_init(void *arg __unused);

/**
 * Send an Interprocessor Interrupt.
 *
 * @param id The CPU ID to send the IPI to
 * @param irq The IRQ to send to the CPU
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_send_ipi(__u64 id, unsigned long irq);

#endif /* CONFIG_HAVE_SMP */
#endif /* CONFIG_LIBUKPLAT_NATIVE_LCPU */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_LCPU_H__ */
