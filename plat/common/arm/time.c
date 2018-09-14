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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */
#include <stdlib.h>
#include <uk/assert.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <uk/arch/atomic.h>
#include <cpu.h>

static uint64_t boot_ticks;
static uint32_t counter_freq;

/*
 * Shift factor for counter scaling multiplier; referred to as S in the
 * following comments.
 */
static uint8_t counter_shift;

/* Multiplier for converting counter ticks to nsecs. (0.S) fixed point. */
static uint32_t ns_per_tick;

/* How many nanoseconds per second */
#define NSEC_PER_SEC ukarch_time_sec_to_nsec(1)

static inline uint64_t ticks_to_ns(uint64_t ticks)
{
	return (ns_per_tick * ticks) >> counter_shift;
}

static inline uint32_t get_counter_frequency(void)
{
	return SYSREG_READ32(cntfrq_el0);
}

static inline uint64_t read_virtual_count(void)
{
	return SYSREG_READ64(cntvct_el0);
}

/*
 * monotonic_clock(): returns # of nanoseconds passed since
 * generic_timer_time_init()
 */
static __nsec generic_timer_monotonic(void)
{
	return (__nsec)ticks_to_ns(read_virtual_count() - boot_ticks);
}

/*
 * Return epoch offset (wall time offset to monotonic clock start).
 */
static __u64  generic_timer_epochoffset(void)
{
	return 0;
}

static int generic_timer_init(void)
{
	/*
	 * Calculate counter shift factor and scaling multiplier.
	 *
	 * counter_shift (S) needs to be the largest (<=32) shift factor where
	 * the result of the ns_per_tick calculation below fits into uint32_t
	 * without truncation. Note that we disallow an S of zero to ensure
	 * the loop always terminates.
	 *
	 * (0.S) ns_per_tick = NSEC_PER_SEC (S.S) / counter_freq (S.0)
	 */
	uint64_t tmp;

	counter_freq = get_counter_frequency();
	counter_shift = 32;
	do {
		tmp = (NSEC_PER_SEC << counter_shift) / counter_freq;
		if ((tmp & 0xFFFFFFFF00000000L) == 0L)
			ns_per_tick = (uint32_t)tmp;
		else
			counter_shift--;
	} while (counter_shift > 0 && ns_per_tick == 0L);
	UK_BUGON(!ns_per_tick);

	/*
	 * Monotonic time begins at boot_ticks (first read of counter
	 * before calibration).
	 */
	boot_ticks = read_virtual_count();

	return 0;
}

/* return ns since time_init() */
__nsec ukplat_monotonic_clock(void)
{
	return generic_timer_monotonic();
}

/* return wall time in nsecs */
__nsec ukplat_clock_wall(void)
{
	return generic_timer_monotonic() + generic_timer_epochoffset();
}

static int timer_handler(void *arg __unused)
{
	/* Yes, we handled the irq. */
	return 1;
}

/* must be called before interrupts are enabled */
void ukplat_time_init(void)
{
	int rc;

	rc = ukplat_irq_register(0, timer_handler, NULL);
	if (rc < 0)
		UK_CRASH("Failed to register timer interrupt handler\n");

	rc = generic_timer_init();
	if (rc < 0)
		UK_CRASH("Failed to initialize platform time\n");
}
