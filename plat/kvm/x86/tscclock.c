/* SPDX-License-Identifier: ISC */
/*
 * Authors: Dan Williams
 *          Martin Lucina
 *          Ricardo Koller
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2015-2017 IBM
 * Copyright (c) 2016-2017 Docker, Inc.
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Taken from solo5 tscclock.c */

/*-
 * Copyright (c) 2014, 2015 Antti Kantee.  All Rights Reserved.
 * Copyright (c) 2015 Martin Lucina.  All Rights Reserved.
 * Modified for solo5 by Ricardo Koller <kollerr@us.ibm.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/clock_event.h>
#include <uk/intctlr.h>
#include <x86/cpu.h>
#include <uk/timeconv.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/event.h>
#include <uk/migration.h>
#include <uk/bitops.h>
#include <uk/atomic.h>
#include <uk/preempt.h>

#define TIMER_CNTR           0x40
#define TIMER_MODE           0x43
#define TIMER_SEL0           0x00
#define TIMER_LATCH          0x00
#define TIMER_RATEGEN        0x04
#define TIMER_ONESHOT        0x08
#define TIMER_16BIT          0x30
#define TIMER_HZ             1193182

#define	RTC_COMMAND          0x70
#define	RTC_DATA             0x71
#define RTC_NMI_DISABLE      (1<<8)
#define RTC_NMI_ENABLE       0
#define	RTC_SEC              0x00
#define	RTC_MIN              0x02
#define	RTC_HOUR             0x04
#define	RTC_DAY              0x07
#define	RTC_MONTH            0x08
#define	RTC_YEAR             0x09
#define	RTC_STATUS_A         0x0a
#define	RTC_UIP              (1<<7)

/*
 * Compile-time check to make sure we don't tick faster than the PIT can go.
 * This is really only a basic sanity check. We'll run into serious issues WAY
 * earlier.
 */
#if TIMER_HZ / CONFIG_HZ < 1
#error Timer tick frequency (CONFIG_HZ) cannot be higher than PIT frequency!
#endif

/* RTC wall time offset at monotonic time base. */
static __u64 rtc_epochoffset;

/*
 * TSC clock specific.
 */

/* Base time values at the last call to tscclock_monotonic(). */
static __u64 time_base;
static __u64 tsc_base;

/* Frequency of the TSC */
static __u64 tsc_freq;

/* Multiplier for converting TSC ticks to nsecs. (0.32) fixed point. */
static __u32 tsc_mult;

/* Shift for TSC tick to nanosecond conversions. This ensures that the tsc_mult
 * multiplication won't overflow
 */
static __s8 tsc_shift;

/*
 * Multiplier for converting nsecs to PIT ticks. (1.32) fixed point.
 *
 * Calculated as:
 *
 *     f = UKARCH_NSEC_PER_SEC / TIMER_HZ   (0.31) fixed point.
 *     pit_mult = 1 / f                     (1.32) fixed point.
 */
static const __u32 pit_mult =
	(1ULL << 63) / ((UKARCH_NSEC_PER_SEC << 31) / TIMER_HZ);


/*
 * Read the current i8254 channel 0 tick count.
 */
static unsigned int i8254_gettick(void)
{
	__u16 rdval;

	outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	rdval  = inb(TIMER_CNTR);
	rdval |= (inb(TIMER_CNTR) << 8);
	return rdval;
}

/*
 * Delay for approximately n microseconds using the i8254 channel 0 counter.
 * Timer must be programmed appropriately before calling this function.
 */
static void i8254_delay(unsigned int n)
{
	unsigned int cur_tick, initial_tick;
	int remaining;
	const unsigned long timer_rval = TIMER_HZ / CONFIG_HZ;

	initial_tick = i8254_gettick();

	remaining = (unsigned long long) n * TIMER_HZ / 1000000;

	while (remaining > 1) {
		cur_tick = i8254_gettick();
		if (cur_tick > initial_tick)
			remaining -= timer_rval - (cur_tick - initial_tick);
		else
			remaining -= initial_tick - cur_tick;
		initial_tick = cur_tick;
	}
}

/*
 * Read a RTC register. Due to PC platform braindead-ness also disables NMI.
 */
static inline __u8 rtc_read(__u8 reg)
{
	outb(RTC_COMMAND, reg | RTC_NMI_DISABLE);
	return inb(RTC_DATA);
}

/*
 * Return current RTC time. Note that due to waiting for the update cycle to
 * complete, this call may take some time.
 */
static __u64 rtc_gettimeofday(void)
{
	struct uktimeconv_bmkclock dt;
	unsigned long flags;

	flags = ukplat_lcpu_save_irqf();

	/*
	 * If RTC_UIP is down, we have at least 244us to obtain a
	 * consistent reading before an update can occur.
	 */
	while (rtc_read(RTC_STATUS_A) & RTC_UIP)
		continue;

	dt.dt_sec = uktimeconv_bcdtobin(rtc_read(RTC_SEC));
	dt.dt_min = uktimeconv_bcdtobin(rtc_read(RTC_MIN));
	dt.dt_hour = uktimeconv_bcdtobin(rtc_read(RTC_HOUR));
	dt.dt_day = uktimeconv_bcdtobin(rtc_read(RTC_DAY));
	dt.dt_mon = uktimeconv_bcdtobin(rtc_read(RTC_MONTH));
	dt.dt_year = uktimeconv_bcdtobin(rtc_read(RTC_YEAR)) + 2000;

	ukplat_lcpu_restore_irqf(flags);

	return uktimeconv_bmkclock_to_nsec(&dt);
}

/**
 * Convert a monotonic clock ticks (ns) to TSC cycles
 */
__u64 tscclock_ns_to_tsc_delta(__nsec ts)
{
	uint32_t gcd, factor, divisor;
	uint64_t rem, sbintime;

	/* Convert time delta into a (32.32) fixed-point number of seconds.
	 * Calculation based on FreeBSD's nstosbt()
	 */
	gcd = 512; /* gcd(UKARCH_NSEC_PER_SEC, 1 << 32) */
	factor = (1UL << 32) / gcd;
	divisor = UKARCH_NSEC_PER_SEC / gcd;
	rem = ts % divisor;
	sbintime =
	    ts / divisor * factor + (rem * factor + divisor - 1) / divisor;

	/* sbintime (32.32) * tsc_freq (32.0)
	 * = tsc (64.32) [(64.0) with mul64_32]
	 */
	return mul64_32(sbintime, tsc_freq);
}

/*
 * Minimum delta to sleep using PIT. Programming seems to have an overhead of
 * 3-4us, but play it safe here.
 */
#define PIT_MIN_DELTA	16

static int tscclock_pit_set_next_event(struct uk_clock_event *ce, __nsec at)
{
	__u64 now, delta_ns;
	__u64 delta_ticks;
	unsigned int ticks;

	now = ukplat_monotonic_clock();

	if (at <= now)
		goto immediate_expire;

	/*
	 * Compute delta in PIT ticks. Return if it is less than minimum safe
	 * amount of ticks.  Essentially this will cause us to spin until
	 * the timeout.
	 */
	delta_ns = at - now;
	delta_ticks = mul64_32(delta_ns, pit_mult);
	if (delta_ticks < PIT_MIN_DELTA)
		goto immediate_expire;

	/*
	 * Program the timer to interrupt the CPU after the delay has expired.
	 * Maximum timer delay is 65535 ticks.
	 */
	if (delta_ticks > 65535)
		ticks = 65535;
	else
		ticks = delta_ticks;

	/*
	 * Note that according to the Intel 82C54 datasheet, p12 the
	 * interrupt is actually delivered in N + 1 ticks.
	 */
	ticks -= 1;
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_ONESHOT | TIMER_16BIT);
	outb(TIMER_CNTR, ticks & 0xff);
	outb(TIMER_CNTR, ticks >> 8);

	return 0;

immediate_expire:
	if (ce->event_handler)
		ce->event_handler(ce);

	return 0;
}

static int tscclock_pit_disable(struct uk_clock_event *ce __unused)
{
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_ONESHOT | TIMER_16BIT);
	outb(TIMER_CNTR, 0);
	outb(TIMER_CNTR, 0);

	return 0;
}

static int pit_timer_handler(void *arg)
{
	struct uk_clock_event *ce = arg;

	tscclock_pit_disable(ce);
	if (ce->event_handler)
		ce->event_handler(ce);

	return 1;
}

static struct uk_clock_event pit_clock_event = {
	.name = "PIT",
	.priority = 0,
	.set_next_event = tscclock_pit_set_next_event,
	.disable = tscclock_pit_disable,
};

/*
 * Return monotonic time using TSC clock.
 */
__u64 tscclock_monotonic(void)
{
	__u64 tsc_now, tsc_delta;

	/*
	 * Update time_base (monotonic time) and tsc_base (TSC time).
	 */
	tsc_now = rdtsc();
	tsc_delta = tsc_now - tsc_base;
	if (tsc_shift >= 0)
		tsc_delta <<= tsc_shift;
	else
		tsc_delta >>= -tsc_shift;

	if (tsc_delta >= UINT64_MAX / 2)
		tsc_delta = 1;
	time_base += mul64_32(tsc_delta, tsc_mult);
	tsc_base = tsc_now;

	return time_base;
}

/* Ensure there is no accidental padding in the following structures */
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wpadded"
struct pvclock_vcpu_time_info {
	__u32	version;
	__u32	pad0;
	__u64	tsc_timestamp;
	__u64	system_time;
	__u32	tsc_to_system_mul;
	__s8	tsc_shift;
	__u8	flags;
	__u8	pad[2];
};

struct pvclock_wall_clock {
	__u32   version;
	__u32   sec;
	__u32   nsec;
};
#pragma GCC diagnostic pop

static struct {
	unsigned int system_clock_msr;
	struct pvclock_vcpu_time_info system_clock;
	unsigned int wall_clock_msr;
	struct pvclock_wall_clock wall_clock;
} pvclock_state;

#define X86_KVM_CPUID_HYPERVISOR		0x40000000
#define X86_KVM_CPUID_FEATURES			0x40000001

#define X86_KVM_FEATURE_CLOCKSOURCE		(0 << 1)
#define X86_KVM_FEATURE_CLOCKSOURCE2		(3 << 1)

#define KVM_PVCLOCK_LEGACY_WALL_CLOCK_MSR	0x11
#define KVM_PVCLOCK_LEGACY_SYS_CLOCK_MSR	0x12
#define KVM_PVCLOCK_WALL_CLOCK_MSR		0x4b564d00
#define KVM_PVCLOCK_SYS_CLOCK_MSR		0x4b564d01

/**
 * Initialize the TSC values from a set of pvclock MSRs.
 * Documentation of these MSRs can ben found at:
 *  https://www.kernel.org/doc/html/v5.9/virt/kvm/msr.html
 */
static void pvclock_init(unsigned int systemclock_msr,
			 unsigned int wallclock_msr)
{
	__u32 version;

	/* Give KVM the address of our pvclock structure and trigger an update.
	 */
	wrmsrl(systemclock_msr, (__u64)&pvclock_state.system_clock | 0x1);
	wrmsrl(wallclock_msr, (__u64)&pvclock_state.wall_clock);

	/* Attempt to fetch a consistent set of values (as in the version did
	 * not change)
	 */
	do {
		version = uk_load_n(&pvclock_state.system_clock.version);

		time_base = pvclock_state.system_clock.system_time;
		tsc_base = pvclock_state.system_clock.tsc_timestamp;
		tsc_mult = pvclock_state.system_clock.tsc_to_system_mul;
		tsc_shift = pvclock_state.system_clock.tsc_shift;
	} while (version != uk_load_n(&pvclock_state.system_clock.version) ||
		 (version & 1));

	do {
		version = uk_load_n(&pvclock_state.wall_clock.version);
		rtc_epochoffset =
		    ukarch_time_sec_to_nsec(pvclock_state.wall_clock.sec)
		    + pvclock_state.wall_clock.nsec;
	} while (version != uk_load_n(&pvclock_state.wall_clock.version) ||
		 (version & 1));

	tsc_freq = (UKARCH_NSEC_PER_SEC << 32) / tsc_mult;
	if (tsc_shift < 0)
		tsc_freq <<= -tsc_shift;
	else
		tsc_freq >>= tsc_shift;

	pvclock_state.system_clock_msr = systemclock_msr;
	pvclock_state.wall_clock_msr = wallclock_msr;
}

static int tscclock_resync(void *arg __unused)
{
	if (pvclock_state.system_clock_msr && pvclock_state.wall_clock_msr) {
		pvclock_init(pvclock_state.system_clock_msr,
			     pvclock_state.wall_clock_msr);
	} else {
		__u64 rtc_current;

		// Update time_base and get a current RTC timestamp
		uk_preempt_disable();
		ukplat_monotonic_clock();
		rtc_current = rtc_gettimeofday();
		uk_preempt_enable();

		rtc_epochoffset = rtc_current - time_base;
	}
	return UK_EVENT_HANDLED_CONT;
}

UK_EVENT_HANDLER(UK_MIGRATION_EVENT, tscclock_resync);

void tscclock_pit_init(void)
{
	__u32 eax, ebx, ecx, edx;
	__u64 rtc_boot;

	/*
	 * Read RTC "time at boot". This must be done just before tsc_base is
	 * initialised in order to get a correct offset below.
	 */
	rtc_boot = rtc_gettimeofday();

	/*
	 * Attempt to retrieve TSC frequency via the hypervisor generic cpuid
	 * timing information leaf. 0x40000010 returns the (virtual) TSC
	 * frequency in kHz, or 0 if the feature is not supported by the
	 * hypervisor.
	 */
	cpuid(0x40000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x40000010) {
		uk_pr_info("Retrieving TSC clock frequency from hypervisor\n");
		tsc_base = rdtsc();
		cpuid(0x40000010, 0, &eax, &ebx, &ecx, &edx);
		tsc_freq = eax * 1000;
	}

	/*
	 * If we could not retrieve the TSC frequency from the hypervisor,
	 * calibrate against an 0.1s delay using the i8254 timer. This is
	 * undesirable as it delays the boot sequence.
	 */
	if (!tsc_freq) {
		uk_pr_info("Calibrating TSC clock against i8254 timer\n");
		tsc_base = rdtsc();
		i8254_delay(100000);
		tsc_freq = (rdtsc() - tsc_base) * 10;
	}

	/*
	 * Calculate TSC scaling multiplier.
	 *
	 * (0.32) tsc_mult = UKARCH_NSEC_PER_SEC (32.32) / tsc_freq (32.0)
	 *
	 * Warning, do not print anything between TSC calibration and the
	 * setting of tsc_mult: if CONFIG_LIBUKDEBUG_PRINT_TIME is enabled
	 * this will trigger a reset of tsc_base via tscclock_monotonic
	 * and delay the clock starting point.
	 *
	 * FIXME: this will overflow with small TSC frequencies. We should
	 * probably calculate the TSC shift dynamically like solo5/hvt does.
	 */
	tsc_mult = (UKARCH_NSEC_PER_SEC << 32) / tsc_freq;

	uk_pr_info("Clock source: TSC, frequency estimate is %llu Hz\n",
		   (unsigned long long) tsc_freq);

	/*
	 * Monotonic time begins at tsc_base (first read of TSC before
	 * calibration).
	 */
	tscclock_monotonic();

	/*
	 * Compute RTC epoch offset by subtracting monotonic time_base from RTC
	 * time at boot.
	 */
	rtc_epochoffset = rtc_boot - time_base;
}

/*
 * Calibrate TSC and initialise TSC clock.
 */
int tscclock_init(void)
{
	__u32 eax, ebx, ecx, edx;
	int rc;

	/* Initialise i8254 timer channel 0 to mode 2 at CONFIG_HZ frequency */
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) & 0xff);
	outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) >> 8);

	/* First try to fetch all timing information from the pvclock */
	cpuid(X86_KVM_CPUID_HYPERVISOR, 0,
	      &eax, &ebx, &ecx, &edx);
	/* The values in the registers correspond to "KVMKVMKVM" */
	if (ebx == 0x4b4d564b && ecx == 0x564b4d56 && edx == 0x4d) {
		cpuid(X86_KVM_CPUID_FEATURES, 0,
		      &eax, &ebx, &ecx, &edx);
		if (eax & X86_KVM_FEATURE_CLOCKSOURCE2) {
			uk_pr_debug("Using modern pvclock to init clock\n");
			pvclock_init(KVM_PVCLOCK_SYS_CLOCK_MSR,
				     KVM_PVCLOCK_WALL_CLOCK_MSR);
		} else if (eax & X86_KVM_FEATURE_CLOCKSOURCE2) {
			uk_pr_debug("Using legacy pvclock to init clock\n");
			pvclock_init(KVM_PVCLOCK_LEGACY_SYS_CLOCK_MSR,
				     KVM_PVCLOCK_LEGACY_WALL_CLOCK_MSR);
		} else {
			uk_pr_debug("KVM does not provide a pvclock\n");
		}
	}

	/* If that fails, use legacy devices */
	if (!tsc_mult)
		tscclock_pit_init();

	/* Register PIT clock event */
	pit_clock_event.disable(&pit_clock_event);
	rc = uk_intctlr_irq_register(ukplat_time_get_irq(), pit_timer_handler,
				     &pit_clock_event);
	if (rc < 0)
		UK_CRASH("Failed to register timer interrupt handler\n");
	uk_clock_event_register(&pit_clock_event);

	return 0;
}

/*
 * Return epoch offset (wall time offset to monotonic clock start).
 */
__u64 tscclock_epochoffset(void)
{
	return rtc_epochoffset;
}

/*
 * Returns early if any interrupts are serviced, or if the requested delay is
 * too short. Must be called with interrupts disabled, will enable interrupts
 * "atomically" during idle loop.
 *
 * This function must be called only from the scheduler. It will screw
 * your system if you do otherwise. And, there is no reason you
 * actually want to use it anywhere else. THIS IS NOT A YIELD or any
 * kind of mutex_lock. It will simply halt the cpu, not allowing any
 * other thread to execute.
 */
static void tscclock_cpu_block(__u64 until)
{
	UK_ASSERT(ukplat_lcpu_irqs_disabled());
	/* If this function is used then we can't use the PIT for other purposes
	 */
	UK_ASSERT(pit_clock_event.event_handler == NULL);

	pit_clock_event.set_next_event(&pit_clock_event, until);

	/*
	 * Wait for any interrupt. If we got an interrupt then just
	 * return into the scheduler (this func is called _ONLY_ from
	 * a scheduler, see the note above) which will check if there
	 * is work to do and call us again here if not.
	 *
	 * TODO: It would be more efficient for longer sleeps to be
	 * able to distinguish if the interrupt was the PIT interrupt
	 * and no other, but this will do for now.
	 */
	ukplat_lcpu_halt_irq();
}

unsigned long sched_have_pending_events;

void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		tscclock_cpu_block(until);

		if (__uk_test_and_clear_bit(0, &sched_have_pending_events))
			break;
	}
}
