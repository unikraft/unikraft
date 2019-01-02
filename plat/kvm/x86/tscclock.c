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
#include <x86/cpu.h>
#include <uk/timeconv.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/bitops.h>

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

/*
 * Beturn monotonic time using TSC clock.
 */
__u64 tscclock_monotonic(void)
{
	__u64 tsc_now, tsc_delta;

	/*
	 * Update time_base (monotonic time) and tsc_base (TSC time).
	 */
	tsc_now = rdtsc();
	tsc_delta = tsc_now - tsc_base;
	time_base += mul64_32(tsc_delta, tsc_mult);
	tsc_base = tsc_now;

	return time_base;
}

/*
 * Calibrate TSC and initialise TSC clock.
 */
int tscclock_init(void)
{
	__u64 tsc_freq, rtc_boot;

	/* Initialise i8254 timer channel 0 to mode 2 at CONFIG_HZ frequency */
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) & 0xff);
	outb(TIMER_CNTR, (TIMER_HZ / CONFIG_HZ) >> 8);

	/*
	 * Read RTC "time at boot". This must be done just before tsc_base is
	 * initialised in order to get a correct offset below.
	 */
	rtc_boot = rtc_gettimeofday();

	/*
	 * Calculate TSC frequency by calibrating against an 0.1s delay
	 * using the i8254 timer.
	 * TODO: Find a more elegant solution that does not require us to
	 * to delay the boot for 100ms. Does KVM provides us a pre-calculated
	 * TSC value?
	 */
	tsc_base = rdtsc();
	i8254_delay(100000);
	tsc_freq = (rdtsc() - tsc_base) * 10;
	uk_pr_info("Clock source: TSC, frequency estimate is %llu Hz\n",
		   (unsigned long long) tsc_freq);

	/*
	 * Calculate TSC scaling multiplier.
	 *
	 * (0.32) tsc_mult = UKARCH_NSEC_PER_SEC (32.32) / tsc_freq (32.0)
	 */
	tsc_mult = (UKARCH_NSEC_PER_SEC << 32) / tsc_freq;

	/*
	 * Monotonic time begins at tsc_base (first read of TSC before
	 * calibration).
	 */
	time_base = mul64_32(tsc_base, tsc_mult);

	/*
	 * Compute RTC epoch offset by subtracting monotonic time_base from RTC
	 * time at boot.
	 */
	rtc_epochoffset = rtc_boot - time_base;

	/*
	 * Initialise i8254 timer channel 0 to mode 4 (one shot).
	 */
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_ONESHOT | TIMER_16BIT);

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
	__u64 now, delta_ns;
	__u64 delta_ticks;
	unsigned int ticks;

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	now = ukplat_monotonic_clock();

	/*
	 * Compute delta in PIT ticks. Return if it is less than minimum safe
	 * amount of ticks.  Essentially this will cause us to spin until
	 * the timeout.
	 */
	delta_ns = until - now;
	delta_ticks = mul64_32(delta_ns, pit_mult);
	if (delta_ticks < PIT_MIN_DELTA) {
		/*
		 * Since we are "spinning", quickly enable interrupts in
		 * the hopes that we might get new work and can do something
		 * else than spin.
		 */
		ukplat_lcpu_enable_irq();
		nop(); /* ints are enabled 1 instr after sti */
		ukplat_lcpu_disable_irq();
		return;
	}

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
	outb(TIMER_CNTR, ticks & 0xff);
	outb(TIMER_CNTR, ticks >> 8);

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
