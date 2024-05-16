/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <uk/assert.h>
#include <uk/plat/time.h>
#include <uk/plat/lcpu.h>
#include <uk/bitops.h>
#include <uk/init.h>
#include <uk/plat/common/cpu.h>
#include <uk/clock_event.h>
#include <arm/time.h>

static uint64_t boot_ticks;
static uint32_t counter_freq;

/* Shift factor for converting ticks to ns */
static uint8_t counter_shift_to_ns;

/* Shift factor for converting ns to ticks */
static uint8_t counter_shift_to_tick;

/* Multiplier for converting counter ticks to nsecs */
static uint32_t ns_per_tick;

/* Multiplier for converting nsecs to counter ticks */
static uint32_t tick_per_ns;

/* Total (absolute) number of nanoseconds per tick */
static uint64_t tot_ns_per_tick;

/*
 * The maximum time range in seconds which can be converted by multiplier
 * and shift factors. This will guarantee the converted value not to exceed
 * 64-bit unsigned integer. Increase the time range will reduce the accuracy
 * of conversion, because we will get smaller multiplier and shift factors.
 * In this case, we selected 3600s as the time range.
 */
#define __MAX_CONVERT_SECS	(3600UL)
#define __MAX_CONVERT_NS	(3600UL*NSEC_PER_SEC)
static uint64_t max_convert_ticks;

/* How many nanoseconds per second */
#define NSEC_PER_SEC ukarch_time_sec_to_nsec(1)

static inline uint64_t ticks_to_ns(uint64_t ticks)
{
	if (ticks > max_convert_ticks) {
		/* We have reached the maximum number of ticks to convert using
		 * the shift factor
		 */
		return (ticks * tot_ns_per_tick);
	} else {
		return (ns_per_tick * ticks) >> counter_shift_to_ns;
	}
}

static inline uint64_t ns_to_ticks(uint64_t ns)
{
	if (ns > __MAX_CONVERT_NS) {
		/* We have reached the maximum number of ns to convert using the
		 * shift factor
		 */
		return (ns / tot_ns_per_tick);
	} else {
		return (tick_per_ns * ns) >> counter_shift_to_tick;
	}
}

/*
 * Calculate multiplier/shift factors for scaled math.
 */
static void calculate_mult_shift(uint32_t *mult, uint8_t *shift,
		uint64_t from, uint64_t to)
{
	uint64_t tmp;
	uint32_t sft, sftacc = 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((uint64_t)__MAX_CONVERT_SECS * from) >> 32;
	while (tmp) {
		tmp >>= 1;
		sftacc--;
	}


	/*
	 * Calculate shift factor (S) and scaling multiplier (M).
	 *
	 * (S) needs to be the largest shift factor (<= max_shift) where
	 * the result of the M calculation below fits into uint32_t
	 * without truncation.
	 *
	 * multiplier = (target << shift) / source
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (uint64_t) to << sft;

		/* Ensuring we round to nearest when calculating the
		 * multiplier
		 */
		tmp += from / 2;
		tmp /= from;
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

void generic_timer_enable(void)
{
	set_el0(cntv_ctl, get_el0(cntv_ctl) | GT_TIMER_ENABLE);

	/* Ensure the write of sys register is visible */
	isb();
}

static inline void generic_timer_disable(void)
{
	set_el0(cntv_ctl, get_el0(cntv_ctl) & ~GT_TIMER_ENABLE);

	/* Ensure the write of sys register is visible */
	isb();
}

static inline void generic_timer_update_compare(uint64_t new_val)
{
	set_el0(cntv_cval, new_val);

	/* Ensure the write of sys register is visible */
	isb();
}

#ifdef CONFIG_ARM64_ERRATUM_858921
/*
 * The errata #858921 describes that Cortex-A73 (r0p0 - r0p2) counter
 * read can return a wrong value when the counter crosses a 32bit boundary.
 * But newer Cortex-A73 are not affected.
 *
 * The workaround involves performing the read twice, compare bit[32] of
 * the two read values. If bit[32] is different, keep the first value,
 * otherwise keep the second value.
 */
uint64_t generic_timer_get_ticks(void)
{
	uint64_t val_1st, val_2nd;

	val_1st = get_el0(cntvct);
	val_2nd = get_el0(cntvct);
	return (((val_1st ^ val_2nd) >> 32) & 1) ? val_1st : val_2nd;
}
#else
uint64_t generic_timer_get_ticks(void)
{
	return get_el0(cntvct);
}
#endif

void generic_timer_update_boot_ticks(void)
{
	boot_ticks = generic_timer_get_ticks();
}

/*
 * monotonic_clock(): returns # of nanoseconds passed since
 * generic_timer_time_init()
 */
static inline __nsec generic_timer_monotonic(void)
{
	return (__nsec)ticks_to_ns(generic_timer_get_ticks() - boot_ticks);
}

/*
 * Return epoch offset (wall time offset to monotonic clock start).
 */
static inline uint64_t generic_timer_epochoffset(void)
{
	return 0;
}

static inline __nsec generic_timer_monotonic_ticks(uint64_t *ticks)
{
	*ticks = generic_timer_get_ticks();
	return ticks_to_ns(*ticks - boot_ticks);
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
void generic_timer_cpu_block_until(uint64_t until_ns)
{
	uint64_t now_ns, now_ticks, until_ticks;

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	/* Record current ns */
	now_ns = generic_timer_monotonic_ticks(&now_ticks);

	if (now_ns < until_ns) {
		/* Calculate until_ticks for timer */
		until_ticks = now_ticks
			+ ns_to_ticks(until_ns - now_ns);
		generic_timer_update_compare(until_ticks);
		generic_timer_enable();
		generic_timer_unmask_irq();
		__asm__ __volatile__("wfi");

		/* Give the IRQ handler a chance to handle whatever woke
		 * us up
		 */
		ukplat_lcpu_enable_irq();
		ukplat_lcpu_disable_irq();
	}
}

int generic_timer_set_next_event(struct uk_clock_event *ce, __nsec at)
{
	uint64_t now_ns, now_ticks, until_ticks;

	/* Record current ns */
	now_ns = generic_timer_monotonic_ticks(&now_ticks);

	if (now_ns < at) {
		/* Calculate until_ticks for timer */
		until_ticks = now_ticks + ns_to_ticks(at - now_ns);
		generic_timer_update_compare(until_ticks);
		generic_timer_enable();
		generic_timer_unmask_irq();
	} else {
		if (ce->event_handler)
			ce->event_handler(ce);
	}

	return 0;
}

int generic_timer_disable_ce(struct uk_clock_event *ce __unused)
{
	generic_timer_mask_irq();
	return 0;
}

static struct uk_clock_event generic_timer_clock_event = {
	.name = "generic_timer",
	.priority = 100,
	.set_next_event = generic_timer_set_next_event,
	.disable = generic_timer_disable_ce,
};

int generic_timer_init(int fdt_timer)
{
	/* Get counter frequency from DTB or register (in Hz) */
	counter_freq = generic_timer_get_frequency(fdt_timer);

	/* Absolute number of ns per tick */
	tot_ns_per_tick = NSEC_PER_SEC / counter_freq;

	/*
	 * Calculate the shift factor and scaling multiplier for
	 * converting ticks to ns.
	 */
	calculate_mult_shift(&ns_per_tick, &counter_shift_to_ns,
				counter_freq, NSEC_PER_SEC);

	/* We disallow zero ns_per_tick */
	UK_BUGON(!ns_per_tick);

	/*
	 * Calculate the shift factor and scaling multiplier for
	 * converting ns to ticks.
	 */
	calculate_mult_shift(&tick_per_ns, &counter_shift_to_tick,
				NSEC_PER_SEC, counter_freq);

	/* We disallow zero ns_per_tick */
	UK_BUGON(!tick_per_ns);

	max_convert_ticks = __MAX_CONVERT_SECS*counter_freq;

	/* Register timer as clock event device */
	uk_clock_event_register(&generic_timer_clock_event);

	return 0;
}

int generic_timer_irq_handler(void *arg __unused)
{
	/*
	 * We just mask the IRQ here, the scheduler will call
	 * generic_timer_cpu_block_until, and then unmask the IRQ.
	 */
	generic_timer_mask_irq();

	if (generic_timer_clock_event.event_handler)
		generic_timer_clock_event.event_handler(
			&generic_timer_clock_event);

	/* Yes, we handled the irq. */
	return 1;
}

/* return ns since time_init() */
__nsec ukplat_monotonic_clock(void)
{
	return generic_timer_monotonic();
}

/* return wall time in nsecs */
__nsec ukplat_wall_clock(void)
{
	return generic_timer_monotonic() + generic_timer_epochoffset();
}
