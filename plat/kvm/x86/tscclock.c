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

#include <stdint.h>

#include <kvm/tscclock.h>
#include <uk/arch/util.h>
#include <uk/lcpu.h>
#include <uk/pcpuvar.h>
#include <uk/plat/config.h>
#include <uk/plat/spinlock.h>
#include <uk/plat/time.h>
#include <uk/timeconv.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/atomic.h>

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

/* TSC value recorded at monotonic clock start (immutable after init). */
static __u64 tsc_boot;

/* Multiplier for converting TSC ticks to nsecs. (0.32) fixed point. */
static __u32 tsc_mult;

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

	uk_arch_x86_64_outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	rdval  = uk_arch_x86_64_inb(TIMER_CNTR);
	rdval |= (uk_arch_x86_64_inb(TIMER_CNTR) << 8);
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
	uk_arch_x86_64_outb(RTC_COMMAND, reg | RTC_NMI_DISABLE);
	return uk_arch_x86_64_inb(RTC_DATA);
}

/*
 * Return current RTC time. Note that due to waiting for the update cycle to
 * complete, this call may take some time.
 */
static __u64 rtc_gettimeofday(void)
{
	struct uktimeconv_bmkclock dt;
	unsigned long flags;

	flags = uk_lcpu_save_irqf();

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

	uk_lcpu_restore_irqf(flags);

	return uktimeconv_bmkclock_to_nsec(&dt);
}

/*
 * Return monotonic time using TSC clock.
 */
__u64 tscclock_monotonic(void)
{
	__u64 tsc_now, tsc_delta;
	__u32 mult = UK_READ_ONCE(tsc_mult);

	if (unlikely(!mult))
		return 0;

	tsc_now = uk_arch_x86_64_rdtsc();
	tsc_delta = tsc_now - tsc_boot;
	if (unlikely(tsc_delta >= UINT64_MAX / 2))
		tsc_delta = 0;

	return (__u64)(((__uint128_t)tsc_delta * mult) >> 32);
}

/*
 * Calibrate TSC and initialise TSC clock.
 */
int tscclock_init(void)
{
	__u64 tsc_freq = 0, rtc_boot, cal_start;
	__u32 eax, ebx, ecx, edx;

	/* Initialise i8254 timer channel 0 to mode 2 at CONFIG_HZ frequency */
	uk_arch_x86_64_outb(TIMER_MODE,
			    TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	uk_arch_x86_64_outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) & 0xff);
	uk_arch_x86_64_outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) >> 8);

	/*
	 * Read RTC "time at boot". This must be done just before tsc_boot is
	 * initialised in order to get a correct offset below.
	 */
	rtc_boot = rtc_gettimeofday();

	/*
	 * Attempt to retrieve TSC frequency via the hypervisor generic cpuid
	 * timing information leaf. 0x40000010 returns the (virtual) TSC
	 * frequency in kHz, or 0 if the feature is not supported by the
	 * hypervisor.
	 */
	uk_arch_x86_64_cpuid(0x40000000, 0, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x40000010) {
		uk_pr_info("Retrieving TSC clock frequency from hypervisor\n");
		cal_start = uk_arch_x86_64_rdtsc();
		uk_arch_x86_64_cpuid(0x40000010, 0, &eax, &ebx, &ecx, &edx);
		tsc_freq = (__u64)eax * 1000;
	}

	/*
	 * If we could not retrieve the TSC frequency from the hypervisor,
	 * calibrate against an 0.1s delay using the i8254 timer. This is
	 * undesirable as it delays the boot sequence.
	 */
	if (!tsc_freq) {
		uk_pr_info("Calibrating TSC clock against i8254 timer\n");
		cal_start = uk_arch_x86_64_rdtsc();
		i8254_delay(100000);
		tsc_freq = (uk_arch_x86_64_rdtsc() - cal_start) * 10;
	}

	tsc_boot = cal_start;
	uk_arch_wmb();

	/*
	 * Calculate TSC scaling multiplier.
	 *
	 * (0.32) tsc_mult = UKARCH_NSEC_PER_SEC (32.32) / tsc_freq (32.0)
	 *
	 * FIXME: this will overflow with small TSC frequencies. We should
	 * probably calculate the TSC shift dynamically like solo5/hvt does.
	 */
	UK_WRITE_ONCE(tsc_mult, (UKARCH_NSEC_PER_SEC << 32) / tsc_freq);

	uk_pr_info("Clock source: TSC, frequency estimate is %llu Hz\n",
		   (unsigned long long) tsc_freq);

	/*
	 * Compute RTC epoch offset by subtracting monotonic time from RTC
	 * time at boot.
	 */
	rtc_epochoffset = rtc_boot - tscclock_monotonic();

	/*
	 * Initialise i8254 timer channel 0 to mode 4 (one shot).
	 */
	uk_arch_x86_64_outb(TIMER_MODE,
			    TIMER_SEL0 | TIMER_ONESHOT | TIMER_16BIT);
	uk_arch_x86_64_outb(TIMER_CNTR, 0);
	uk_arch_x86_64_outb(TIMER_CNTR, 0);

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
 * Minimum delta to sleep using PIT. Programming seems to have an overhead of
 * 3-4us, but play it safe here.
 */
#define PIT_MIN_DELTA	16

static inline void i8254_oneshot_program(__u64 delta_ns)
{
	__u64 delta_ticks = (__u64)(((__uint128_t)delta_ns * pit_mult) >> 32);
	unsigned int ticks;

	if (delta_ticks < PIT_MIN_DELTA)
		ticks = PIT_MIN_DELTA;
	else if (delta_ticks > 65535)
		ticks = 65535;
	else
		ticks = (unsigned int)delta_ticks;

	/*
	 * Note that according to the Intel 82C54 datasheet, p12 the
	 * interrupt is actually delivered in N + 1 ticks.
	 */
	ticks -= 1;
	uk_arch_x86_64_outb(TIMER_CNTR, ticks & 0xff);
	uk_arch_x86_64_outb(TIMER_CNTR, ticks >> 8);
}

static __spinlock pit_lock = UKARCH_SPINLOCK_INITIALIZER();
static __u64 pit_cpu_deadline[CONFIG_UKPLAT_CPU_MAXCOUNT];

void tscclock_timer_irq(void)
{
#if CONFIG_HAVE_SMP
	__u64 targets[CONFIG_UKPLAT_CPU_MAXCOUNT];
	unsigned int i, n = 0;

	for (i = 1; i < CONFIG_UKPLAT_CPU_MAXCOUNT; i++) {
		if (UK_READ_ONCE(pit_cpu_deadline[i]) != 0)
			targets[n++] = i;
	}
	if (n > 0)
		(void)uk_lcpu_wakeup(targets, &n);
#endif /* CONFIG_HAVE_SMP */
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
	__u64 cpu_idx = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);
	__u64 now, delta_ns, min_until, pit_delta_ns;
	__u64 delta_ticks;
	unsigned int i;

	UK_ASSERT(uk_lcpu_irqs_disabled());
	UK_ASSERT(cpu_idx < CONFIG_UKPLAT_CPU_MAXCOUNT);

	now = ukplat_monotonic_clock();
	if (until <= now)
		return;

	/*
	 * Compute delta in PIT ticks. Return if it is less than minimum safe
	 * amount of ticks.  Essentially this will cause us to spin until
	 * the timeout.
	 */
	delta_ns = until - now;
	delta_ticks = (__u64)(((__uint128_t)delta_ns * pit_mult) >> 32);
	if (delta_ticks < PIT_MIN_DELTA) {
		/*
		 * Since we are "spinning", quickly enable interrupts in
		 * the hopes that we might get new work and can do something
		 * else than spin.
		 */
		uk_lcpu_enable_irq();
		uk_arch_x86_64_nop(); /* ints are enabled 1 instr after sti */
		uk_lcpu_disable_irq();
		return;
	}

	ukarch_spin_lock(&pit_lock);
	UK_WRITE_ONCE(pit_cpu_deadline[cpu_idx], until);
	min_until = until;
	for (i = 0; i < CONFIG_UKPLAT_CPU_MAXCOUNT; i++) {
		__u64 d = UK_READ_ONCE(pit_cpu_deadline[i]);

		if (d != 0 && d < min_until)
			min_until = d;
	}
	pit_delta_ns = (min_until > now) ? (min_until - now) : 0;
	i8254_oneshot_program(pit_delta_ns);
	ukarch_spin_unlock(&pit_lock);

	/*
	 * Wait for any interrupt. If we got an interrupt then just
	 * return into the scheduler (this func is called _ONLY_ from
	 * a scheduler, see the note above) which will check if there
	 * is work to do and call us again here if not.
	 */
	uk_lcpu_halt_irq();

	ukarch_spin_lock(&pit_lock);
	UK_WRITE_ONCE(pit_cpu_deadline[cpu_idx], 0);
	ukarch_spin_unlock(&pit_lock);
}

__uk_pcpuvar unsigned long sched_have_pending_events;

void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		tscclock_cpu_block(until);

		if (uk_and_relax(uk_pcpuvar_current_ptr_get(
					 sched_have_pending_events), 0))
			break;
	}
}
