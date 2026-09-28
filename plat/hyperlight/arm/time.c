/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Generic-timer based time for Hyperlight guests on arm64.
 *
 * The monotonic clock counts the virtual counter (CNTVCT_EL0) at the
 * frequency in CNTFRQ_EL0, both readable at EL1 on every hypervisor.
 * There is no timer interrupt: the step model hands the vCPU back to the
 * host with the next deadline instead (see time_block_until()).
 *
 * Unlike the x86 TSC, the counter is not part of what a snapshot
 * restores: a restored guest reads the counter of whatever vCPU runs it,
 * which may be behind the one that took the snapshot.  So the clock
 * remembers the last value it returned, and hyperlight_time_restore()
 * re-anchors it there before anything else runs after a restore.  The
 * monotonic clock then carries on from the snapshot, as it does on x86.
 *
 * Wall-clock epoch is obtained from the host via GetWallClockNs at
 * boot so the guest can report real timestamps, and again on a resume
 * (hyperlight_time_resync).  If the host does not register the
 * function, wall time falls back to monotonic.
 */

#include <uk/arch/util.h>
#include <uk/assert.h>
#include <uk/arch/time.h>
#include <uk/atomic.h>
#include <uk/plat/time.h>
#include <uk/print.h>

#include <hyperlight/hcall.h>
#include <hyperlight/step.h>
#include <hyperlight/time.h>

static __u64 cnt_freq;	/* Hz */
static __u64 cnt_start;	/* counter value at monotonic time 0 */

/* The last monotonic time returned, to carry on from after a restore. */
static __nsec mono_last;

/* Wall-clock epoch: ns since Unix epoch at boot, from host */
static __u64 wall_clock_boot_ns;

/* Used by lib/ukintctlr to signal pending events to the scheduler */
unsigned long sched_have_pending_events;

static inline __u64 cnt_read(void)
{
	__u64 v;

	/* The ISB keeps the read from being taken ahead of earlier
	 * instructions, as for any generic-timer read.
	 */
	__asm__ __volatile__("isb; mrs %0, cntvct_el0" : "=r"(v) : : "memory");
	return v;
}

static inline __u64 cnt_to_ns(__u64 cnt)
{
	/* Seconds and remainder apart: cnt * 10^9 overflows after a few
	 * minutes at the usual 24 MHz.
	 */
	return (cnt / cnt_freq) * UKARCH_NSEC_PER_SEC +
	       ((cnt % cnt_freq) * UKARCH_NSEC_PER_SEC) / cnt_freq;
}

/* Rounded up, so that cnt_to_ns() of the result is never below `ns`: the
 * clock a restore re-anchors on must not step back from the last reading.
 */
static inline __u64 ns_to_cnt(__u64 ns)
{
	return (ns / UKARCH_NSEC_PER_SEC) * cnt_freq +
	       ((ns % UKARCH_NSEC_PER_SEC) * cnt_freq +
		UKARCH_NSEC_PER_SEC - 1) / UKARCH_NSEC_PER_SEC;
}

__nsec ukplat_monotonic_clock(void)
{
	__nsec now;

	if (unlikely(!cnt_freq))
		return 0;

	now = cnt_to_ns(cnt_read() - cnt_start);
	mono_last = now;
	return now;
}

__nsec ukplat_wall_clock(void)
{
	return wall_clock_boot_ns + ukplat_monotonic_clock();
}

void hyperlight_time_restore(void)
{
	if (cnt_freq)
		cnt_start = cnt_read() - ns_to_cnt(mono_last);
}

void hyperlight_time_resync_to(__u64 now)
{
	/* The monotonic clock carried on from the snapshot; the epoch it
	 * counts from is what has to move.
	 */
	if (now)
		wall_clock_boot_ns = now - ukplat_monotonic_clock();
}

void hyperlight_time_resync(void)
{
	hyperlight_time_resync_to(hl_call_get_wall_clock_ns());
}

void ukplat_time_init(void)
{
	cnt_start = cnt_read();
	cnt_freq = UK_ARCH_ARM64_SYSREG_READ64(CNTFRQ_EL0);
	if (unlikely(!cnt_freq))
		UK_CRASH("CNTFRQ_EL0 is zero: no counter frequency\n");
	uk_pr_info("Counter frequency: %llu Hz\n",
		   (unsigned long long)cnt_freq);

	/* Query the host for the current wall-clock time.
	 * If the host doesn't register GetWallClockNs, we get 0 and
	 * wall time falls back to monotonic (epoch = guest boot).
	 */
	wall_clock_boot_ns = hl_call_get_wall_clock_ns();
	if (wall_clock_boot_ns)
		uk_pr_info("Wall clock epoch: %llu ns\n",
			   (unsigned long long)wall_clock_boot_ns);
}

void ukplat_time_fini(void)
{
}

__u32 ukplat_time_get_irq(void)
{
	return 0;
}

#ifdef CONFIG_LIBHOSTSOCK
extern int hostsock_rescan_events(void);
#endif

/*
 * Block CPU until the specified time or until pending events arrive.
 *
 * There is no timer interrupt to wake us from WFI, so without the step
 * model this spins on the counter, polling the host sockets every ~1 ms
 * so threads blocked on socket I/O are woken.
 */
void time_block_until(__snsec until)
{
#ifdef CONFIG_LIBHOSTSOCK
	__snsec next_rescan = 0;
#endif
	__snsec now;

	/* Under a step pump the idle thread hands the vCPU back to the
	 * host with this deadline instead of spinning on it; the host
	 * re-enters the guest when it is due (or earlier, on I/O).
	 */
	if (hyperlight_step_halt((__nsec)until))
		return;

	while ((now = (__snsec)ukplat_monotonic_clock()) < until) {
		__asm__ __volatile__("yield");

		if (uk_and_relax(&sched_have_pending_events, 0))
			break;

#ifdef CONFIG_LIBHOSTSOCK
		if (now >= next_rescan) {
			if (hostsock_rescan_events())
				break;
			next_rescan = now + 1000000; /* 1 ms */
		}
#endif
	}
}
