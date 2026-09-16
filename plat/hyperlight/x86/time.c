/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * TSC-based time for Hyperlight guests.
 *
 * Hyperlight preserves TSC, TSC_ADJUST, and TSC_AUX across
 * snapshot/restore, so guest time is consistent across warm runs.
 * Legacy timer hardware (PIT, RTC) is not emulated — even with
 * hw-interrupts enabled, PIT port writes are no-ops and the host
 * uses a timer thread + irqfd instead.
 *
 * TODO: Adopt Hyperlight's paravirtualized guest clock (PR #1422)
 * which provides pre-computed scaling factors via a clock page,
 * eliminating the need for frequency guessing entirely.
 *
 * Wall-clock epoch is obtained from the host via GetWallClockNs at
 * boot so the guest can report real timestamps, and again on a resume
 * (hyperlight_time_resync).  If the host does not register the
 * function, wall time falls back to monotonic.
 */

#include <uk/arch/x86_64.h>
#include <uk/assert.h>
#include <uk/atomic.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/print.h>

#include <hyperlight-x86/hcall.h>
#include <hyperlight-x86/step.h>

/* TSC state */
static __u64 tsc_freq;	/* Hz */
static __u64 tsc_start;

/* Wall-clock epoch: ns since Unix epoch at boot, from host */
static __u64 wall_clock_boot_ns;

/* Used by lib/ukintctlr to signal pending events to the scheduler */
unsigned long sched_have_pending_events;

/*
 * Try CPUID leaf 0x15: TSC / core-crystal clock ratio.
 *   EAX = denominator, EBX = numerator, ECX = crystal Hz (may be 0).
 * Returns frequency in Hz, or 0 if unavailable.
 */
static __u64 tsc_freq_from_cpuid15(void)
{
	__u32 eax, ebx, ecx, edx;

	uk_arch_x86_64_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
	if (eax < 0x15)
		return 0;

	uk_arch_x86_64_cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
	if (!eax || !ebx)
		return 0;

	if (ecx)
		return (__u64)ecx * ebx / eax;

	/* Crystal frequency not reported — can't compute */
	return 0;
}

/*
 * Try CPUID leaf 0x40000010: hypervisor TSC frequency in kHz.
 * Returns frequency in Hz, or 0 if unavailable.
 */
static __u64 tsc_freq_from_hv_cpuid(void)
{
	__u32 eax, ebx, ecx, edx;

	uk_arch_x86_64_cpuid(0x40000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax < 0x40000010)
		return 0;

	uk_arch_x86_64_cpuid(0x40000010, 0, &eax, &ebx, &ecx, &edx);
	if (!eax)
		return 0;

	return (__u64)eax * 1000;
}

static void discover_tsc_freq(void)
{
	tsc_freq = tsc_freq_from_cpuid15();
	if (tsc_freq) {
		uk_pr_info("TSC frequency from CPUID.15H: %llu Hz\n",
			   (unsigned long long)tsc_freq);
		return;
	}

	tsc_freq = tsc_freq_from_hv_cpuid();
	if (tsc_freq) {
		uk_pr_info("TSC frequency from hypervisor CPUID: %llu Hz\n",
			   (unsigned long long)tsc_freq);
		return;
	}

	/* Fallback — no calibration hardware available */
	tsc_freq = 2500000000ULL;
	uk_pr_warn("TSC frequency unknown, assuming 2.5 GHz\n");
}

__nsec ukplat_monotonic_clock(void)
{
	__u64 tsc_delta;
	__u64 secs, rem;

	if (unlikely(!tsc_freq))
		return 0;

	tsc_delta = uk_arch_x86_64_rdtsc() - tsc_start;

	/*
	 * Split into seconds + remainder to avoid overflow.
	 * tsc_delta * 10^9 overflows uint64 after ~7 s at 2.5 GHz.
	 */
	secs = tsc_delta / tsc_freq;
	rem  = tsc_delta % tsc_freq;
	return secs * UKARCH_NSEC_PER_SEC
	     + (rem * UKARCH_NSEC_PER_SEC) / tsc_freq;
}

__nsec ukplat_wall_clock(void)
{
	return wall_clock_boot_ns + ukplat_monotonic_clock();
}

void hyperlight_time_resync(void)
{
	__u64 now = hl_call_get_wall_clock_ns();

	/* The monotonic clock carried on from the snapshot; the epoch it
	 * counts from is what has to move.
	 */
	if (now)
		wall_clock_boot_ns = now - ukplat_monotonic_clock();
}

void ukplat_time_init(void)
{
	tsc_start = uk_arch_x86_64_rdtsc();
	discover_tsc_freq();

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

/*
 * Block CPU until the specified time or until pending events arrive.
 *
 * Without hw-interrupts enabled, there is no timer IRQ to wake us
 * from HLT, so we spin-wait on the TSC instead.
 *
 * When hostsock is compiled in, we periodically poll tracked sockets
 * via hostsock_rescan_events() so threads blocked on socket I/O
 * (accept, recv) get woken when data arrives.  This enables
 * intra-guest networking: a server thread can yield via EAGAIN
 * while a client thread connects and sends data.
 *
 * TODO: When hw-interrupts support is implemented, use HLT with
 * the PvTimer to get proper idle-wait instead of busy-looping.
 */
#ifdef CONFIG_LIBHOSTSOCK
extern int hostsock_rescan_events(void);
#endif

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
		uk_arch_x86_64_nop();

		if (uk_and_relax(&sched_have_pending_events, 0))
			break;

#ifdef CONFIG_LIBHOSTSOCK
		/* Poll tracked sockets every ~1 ms to wake blocked
		 * threads.  Each rescan is a non-blocking net_poll
		 * host call per tracked socket.
		 *
		 * TODO: Replace polling with event-driven wakeup —
		 * host-side epoll thread + irqfd injection would
		 * eliminate the 1 ms latency floor entirely. */
		if (now >= next_rescan) {
			if (hostsock_rescan_events())
				break;
			next_rescan = now + 1000000; /* 1 ms */
		}
#endif
	}
}
