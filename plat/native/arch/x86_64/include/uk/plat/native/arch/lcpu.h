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
 * Halt the current x86_64 CPU (HLT instruction).
 * Stops instruction execution until the next interrupt, NMI, or reset.
 * Does not disable interrupts; typically called with interrupts disabled.
 */
static inline void uk_plat_native_halt(void)
{
	uk_arch_halt();
}

/**
 * Halt the current CPU and wait for an interrupt.
 * Enables interrupts (STI) and immediately halts (HLT) in an atomic sequence
 * to avoid the race condition where an interrupt fires between STI and HLT,
 * which would cause the CPU to halt indefinitely.
 *
 * The x86 STI instruction delays interrupt delivery until after the next
 * instruction, ensuring HLT executes before any interrupt is serviced.
 * Interrupts are disabled again (CLI) after waking from HLT.
 */
static inline void uk_plat_native_halt_irq(void)
{
	/*
	 * We have to be careful when enabling interrupts before entering a
	 * halt state. If we want to wait for an interrupt (e.g., a timer)
	 * the interrupt may fire in the short window between sti and hlt and
	 * we are going to halt forever. As sti only enables interrupts after
	 * the following instruction, we can avoid the race condition by
	 * ensuring that hlt immediately follows sti. There must be no
	 * instruction in between.
	 */
	asm volatile (
		"sti\n\t"
		"hlt\n\t"
		"cli\n\t"
		:
		:
		: "memory"
	);
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
 * Wake up a halted x86_64 CPU.
 * Sends an inter-processor interrupt (IPI) to bring the target CPU out of
 * HLT state. Does not wait for the CPU to resume execution.
 *
 * @param lcpu  Target LCPU to wake up
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_wakeup(struct uk_lcpu *lcpu);

/**
 * Execute a function on a remote x86_64 CPU.
 * Sends an IPI to the target CPU with a function pointer and argument.
 * The function executes in interrupt context on the remote CPU.
 *
 * @param lcpu   Target LCPU to run function on
 * @param fn     Function to execute (with register snapshot and argument)
 * @param flags  Execution flags (unused on x86_64)
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_run(struct uk_lcpu *lcpu, const struct uk_lcpu_func *fn,
			    unsigned long flags __unused);

/**
 * Finalize LCPU startup after initialization.
 * Performs post-start operations for multiple LCPUs, such as synchronization
 * barriers or enabling advanced CPU features.
 *
 * @param lcpuidx  Array of LCPU indices that were started (NULL for all)
 * @param num      [IN] Number of indices in array, [OUT] number processed
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_post_start(const __u32 lcpuidx[],
				   unsigned int *num);

/**
 * Start an x86_64 application processor (AP).
 * Wakes the AP from INIT state using SIPI (Startup IPI) and initializes
 * its execution environment. The AP begins executing at a predefined
 * entry point.
 *
 * @param lcpu   LCPU structure for the AP to start
 * @param flags  Startup flags (unused on x86_64)
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_start(struct uk_lcpu *lcpu,
			      unsigned long flags __unused);

/**
 * Detect and populate LCPU structures for all x86_64 CPUs.
 * Enumerates logical CPUs via ACPI MADT or MP tables and creates
 * corresponding LCPU structures. Called once during boot on BSP.
 *
 * @param arg  Reserved for future use (currently unused)
 * @return 0 on success, negative errno on failure
 */
int uk_plat_native_lcpu_mp_init(void *arg __unused);

#endif /* CONFIG_HAVE_SMP */
#endif /* CONFIG_LIBUKPLAT_NATIVE_LCPU */

#ifdef __cplusplus
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_PLAT_NATIVE_ARCH_LCPU_H__ */
