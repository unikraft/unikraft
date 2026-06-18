/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stdlib.h>
#include <string.h>
#include <uk/plat/time.h>
#include <uk/lcpu.h>
#include <uk/intctlr.h>
#include <uk/assert.h>
#include <uk/atomic.h>
#include <uk/arch/x86_64.h>
#include <hyperlight-x86/setup.h>
#include <hyperlight-x86/hcall.h>

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

/* Wall time in ns since the Unix epoch.
 *
 * Hyperlight guests have no real-time clock — we get the wall time
 * from the host at VM boot (init_data HLWALL0 TLV) and add our own
 * monotonic delta on top. After a snapshot/restore both components
 * roll back, so the guest sees the same "epoch" on every warm run;
 * that's fine for things like xlsx timestamps (>=1980) and logging
 * deltas within a run, but it is NOT a real-time wall clock.
 */
__nsec ukplat_wall_clock(void)
{
	return hyperlight_wall_boot_ns_from_host() + ukplat_monotonic_clock();
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

#ifdef CONFIG_HYPERLIGHT_HCALL
/*
 * Call __hl_sleep via hcall. On the host this now also polls all sockets
 * in the socket table, returning early with "socket_ready":true when a
 * socket event occurs.  Returns 1 if sockets became ready, 0 otherwise.
 */
static int hyperlight_sleep_ns(__u64 ns)
{
	static char _req[128];
	static char _resp[256];
	__sz resp_len = 0;
	int n = snprintf(_req, sizeof(_req),
			 "{\"name\":\"__hl_sleep\",\"args\":{\"ns\":%llu}}",
			 (unsigned long long)ns);
	if (n < 0 || (__sz)n >= sizeof(_req))
		return 0;
	if (hyperlight_hcall((const __u8 *)_req, (__sz)n,
			     (__u8 *)_resp, sizeof(_resp) - 1,
			     &resp_len) < 0)
		return 0;
	_resp[resp_len] = '\0';
	return strstr(_resp, "\"socket_ready\":true") != NULL;
}

#ifdef CONFIG_LIBHOSTSOCK
extern void hostsock_rescan_events(void);
#endif

/* Block CPU until the specified time or pending events.
 *
 * Uses __hl_sleep instead of bare HLT so the host can simultaneously
 * poll sockets and wake us early when network I/O is ready.
 */
void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		__snsec remaining = until - (__snsec)ukplat_monotonic_clock();
		if (remaining <= 0)
			break;

		/* Cap each sleep at 100 ms to keep poll latency bounded. */
		__u64 sleep_ns = (__u64)remaining;
		if (sleep_ns > 100000000ULL)
			sleep_ns = 100000000ULL;

		if (hyperlight_sleep_ns(sleep_ns)) {
#ifdef CONFIG_LIBHOSTSOCK
			hostsock_rescan_events();
#endif
			break;
		}
	}
}
#else
/* Block CPU until the specified time or pending events */
void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		uk_lcpu_halt_irq();

		if (uk_and_relax(&sched_have_pending_events, 0))
			break;
	}
}
#endif
