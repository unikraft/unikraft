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
#include <libfdt.h>
#include <uk/assert.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <uk/bitops.h>
#include <cpu.h>

/* TODO: For now this file is KVM dependent. As soon as we have more
 * Arm platforms that are using this file, we need to introduce a
 * portable way to handover the DTB entry point to common platform code */
#include <kvm/config.h>

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

/*
 * On a few platforms the frequency is not configured correctly
 * by the firmware. A property in the DT (clock-frequency) has
 * been introduced to workaround those firmware. So, we will try
 * to get clock-frequency from DT first, if failed we will read
 * the register directly.
 */
static uint32_t get_counter_frequency(void)
{
	int fdt_archtimer, len;
	const uint64_t *fdt_freq;

	/* Try to find arm,armv8-timer first */
	fdt_archtimer = fdt_node_offset_by_compatible(_libkvmplat_cfg.dtb,
						-1, "arm,armv8-timer");
	/* If failed, try to find arm,armv7-timer */
	if (fdt_archtimer < 0)
		fdt_archtimer = fdt_node_offset_by_compatible(
							_libkvmplat_cfg.dtb,
							-1, "arm,armv7-timer");
	/* DT doesn't provide arch timer information */
	if (fdt_archtimer < 0)
		goto endnofreq;

	fdt_freq = fdt_getprop(_libkvmplat_cfg.dtb,
			fdt_archtimer, "clock-frequency", &len);
	if (!fdt_freq || (len <= 0)) {
		uk_pr_info("No clock-frequency found, reading from register directly.\n");
		goto endnofreq;
	}

	return fdt32_to_cpu(fdt_freq[0]);

endnofreq:
	return SYSREG_READ32(cntfrq_el0);
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
static uint64_t read_virtual_count(void)
{
    uint64_t val_1st, val_2nd;

    val_1st = SYSREG_READ64(cntvct_el0);
    val_2nd = SYSREG_READ64(cntvct_el0);
    return (((val_1st ^ val_2nd) >> 32) & 1) ? val_1st : val_2nd;
}
#else
static inline uint64_t read_virtual_count(void)
{
	return SYSREG_READ64(cntvct_el0);
}
#endif

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

unsigned long sched_have_pending_events;

void time_block_until(__snsec until)
{
	while ((__snsec) ukplat_monotonic_clock() < until) {
		/*
		 * TODO:
		 * As we haven't support interrupt on Arm, so we just
		 * use busy polling for now.
		 */
		if (__uk_test_and_clear_bit(0, &sched_have_pending_events))
			break;
	}
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
