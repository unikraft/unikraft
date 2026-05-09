/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdlib.h>
#include <uk/plat/time.h>
#include <uk/lcpu.h>
#include <uk/intctlr.h>
#include <uk/assert.h>
#include <uk/atomic.h>
#include <uk/arch/x86_64.h>

/* TSC frequency in Hz - will be calibrated at init */
static __u64 tsc_freq;
static __u64 tsc_start;

/* Used by scheduler to signal pending events */
unsigned long sched_have_pending_events;

/* Simple TSC-based monotonic clock */
__nsec ukplat_monotonic_clock(void)
{
	__u64 tsc_now = uk_arch_x86_64_rdtsc();
	__u64 tsc_delta = tsc_now - tsc_start;
	
	if (tsc_freq == 0)
		return 0;
	
	/* Convert TSC ticks to nanoseconds */
	return (tsc_delta * 1000000000ULL) / tsc_freq;
}

/* return wall time in nsecs */
__nsec ukplat_wall_clock(void)
{
	/* For now, just return monotonic time since we don't have RTC */
	return ukplat_monotonic_clock();
}

static int timer_handler(void *arg __unused)
{
	/* Yes, we handled the irq. */
	return 1;
}

/* Estimate TSC frequency using a simple loop.
 * This is a rough estimate - Hyperlight guests don't have access to
 * PIT or other timing hardware for calibration.
 */
static void estimate_tsc_freq(void)
{
	/* Assume a reasonable default frequency of 2.5 GHz 
	 * This can be improved if Hyperlight passes the TSC frequency
	 * to the guest in the PEB.
	 */
	tsc_freq = 2500000000ULL;
}

/* must be called before interrupts are enabled */
void ukplat_time_init(void)
{
	int rc;

	rc = uk_intctlr_irq_register(0, timer_handler, NULL);
	if (rc < 0)
		UK_CRASH("Failed to register timer interrupt handler\n");

	tsc_start = uk_arch_x86_64_rdtsc();
	estimate_tsc_freq();
}

void ukplat_time_fini(void)
{
}

__u32 ukplat_time_get_irq(void)
{
	return 0;
}

/* Block CPU until the specified time or pending events */
void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		/* In Hyperlight, we just halt and wait for interrupt */
		uk_lcpu_halt_irq();

		if (uk_and_relax(&sched_have_pending_events, 0))
			break;
	}
}
