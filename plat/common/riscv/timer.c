/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Wei Chen <Wei.Chen@arm.com>
 *          Eduard Vintila <eduard.vintila47@gmail.com>
 *
 * Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2022, University of Bucharest. All rights reserved.
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
#include <riscv/time.h>
#include <riscv/cpu_defs.h>
#include <riscv/cpu.h>
#include <riscv/sbi.h>
#include <uk/arch/time.h>
#include <uk/plat/lcpu.h>
#include <uk/assert.h>
#include <uk/print.h>
#include <rtc/goldfish.h>
#include <libfdt.h>

/* Frequency at which the time counter CSR is updated */
static __u32 timebase_freq;

/* Shift factor for converting ticks to ns */
static __u8 counter_shift_to_ns;

/* Shift factor for converting ns to ticks */
static __u8 counter_shift_to_tick;

/* Multiplier for converting counter ticks to nsecs */
static __u32 ns_per_tick;

/* Multiplier for converting nsecs to counter ticks */
static __u32 tick_per_ns;

/* Total (absolute) number of nanoseconds per tick */
static __u64 tot_ns_per_tick;

/* Time counter value at boot */
static __u64 boot_ticks;

/* Epoch offset for computing wall time */
static __nsec rtc_epochoffset;

/*
 * The maximum time range in seconds which can be converted by multiplier
 * and shift factors. This will guarantee the converted value not to exceed
 * 64-bit unsigned integer. Increase the time range will reduce the accuracy
 * of conversion, because we will get smaller multiplier and shift factors.
 * In this case, we selected 3600s as the time range.
 */
#define __MAX_CONVERT_SECS	(3600UL)
#define __MAX_CONVERT_NS	(3600UL*NSEC_PER_SEC)
static __u64 max_convert_ticks;

/* How many nanoseconds per second */
#define NSEC_PER_SEC UKARCH_NSEC_PER_SEC


/*
 * Calculate multiplier/shift factors for scaled math.
 */
static void calculate_mult_shift(__u32 *mult, __u8 *shift,
		__u64 from, __u64 to)
{
	__u64 tmp;
	__u32 sft, sftacc = 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((__u64)__MAX_CONVERT_SECS * from) >> 32;
	while (tmp) {
		tmp >>= 1;
		sftacc--;
	}

	/*
	 * Calculate shift factor (S) and scaling multiplier (M).
	 *
	 * (S) needs to be the largest shift factor (<= max_shift) where
	 * the result of the M calculation below fits into __u32
	 * without truncation.
	 *
	 * multiplier = (target << shift) / source
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (__u64) to << sft;

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

/* Get the current time counter ticks */
static __u64 get_timer_ticks(void)
{
	return _csr_read(CSR_TIME);
}

/* Convert time counter ticks to nanoseconds */
static __nsec ticks_to_ns(__u64 ticks)
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

/* Convert nanoseconds to time counter ticks */
static __u64 ns_to_ticks(__nsec ns)
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

/* No. of nanoseconds since boot */
__nsec timer_monotonic_clock(void)
{
	return ticks_to_ns(get_timer_ticks() - boot_ticks);
}

__nsec timer_epoch_offset(void)
{
	return rtc_epochoffset;
}

/*
 * Retrieve the time counter frequency from the device tree.
 * Return 0 if it could not be found.
 */
static __u32 _dtb_get_timer_freq(void *dtb)
{
	const __u32 *freq_prop;
	int cpus_offset = fdt_path_offset(dtb, "/cpus");

	if (cpus_offset < 0)
		return 0;

	freq_prop = fdt_getprop(dtb, cpus_offset, "timebase-frequency", NULL);
	if (!freq_prop)
		return 0;

	return fdt32_to_cpu(freq_prop[0]);
}

void timer_cpu_block_until(__nsec until)
{
	__nsec now_ns;
	__u64 delta_ticks, until_ticks;

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	now_ns = timer_monotonic_clock();
	if (now_ns < until) {
		delta_ticks = ns_to_ticks(until - now_ns);
		until_ticks = get_timer_ticks() + delta_ticks;

		/* Set the time alarm through SBI */
		sbi_set_timer(until_ticks);

		/* Halt the hart until it receives an interrupt */
		ukplat_lcpu_halt_irq();
	}
}

/*
 * Initialize the RISC-V timer.
 * This function MUST be called after RTC initialization in order to compute the
 * RTC epoch offset correctly. Returns 0 on success, -1 on failure.
 */
int init_timer(void *dtb)
{
	__nsec rtc_boot;

	timebase_freq = _dtb_get_timer_freq(dtb);
	if (!timebase_freq)
		return -1;

	rtc_boot = goldfish_read_raw();
	boot_ticks = get_timer_ticks();

	uk_pr_debug("RTC boot: %lu\n", goldfish_read_raw());
	uk_pr_debug("Boot-time ticks: %lu\n", boot_ticks);
	uk_pr_info("Found time counter frequency: %u Hz\n", timebase_freq);

	/* Absolute number of ns per tick */
	tot_ns_per_tick = NSEC_PER_SEC / timebase_freq;

	/*
	 * Calculate the shift factor and scaling multiplier for
	 * converting ticks to ns.
	 */
	calculate_mult_shift(&ns_per_tick, &counter_shift_to_ns,
				timebase_freq, NSEC_PER_SEC);

	/* We disallow zero ns_per_tick */
	UK_BUGON(!ns_per_tick);

	/*
	 * Calculate the shift factor and scaling multiplier for
	 * converting ns to ticks.
	 */
	calculate_mult_shift(&tick_per_ns, &counter_shift_to_tick,
				NSEC_PER_SEC, timebase_freq);

	/* We disallow zero tick_per_ns */
	UK_BUGON(!tick_per_ns);

	max_convert_ticks = __MAX_CONVERT_SECS * timebase_freq;

	rtc_epochoffset = rtc_boot - timer_monotonic_clock();
	return 0;
}
