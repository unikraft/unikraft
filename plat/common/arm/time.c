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
#include <libfdt.h>
#include <uk/ofw/fdt.h>
#include <uk/assert.h>
#include <uk/plat/time.h>
#include <uk/plat/lcpu.h>
#include <uk/bitops.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/sections.h>
#include <uk/plat/common/bootinfo.h>
#include <uk/intctlr.h>
#include <arm/time.h>

static const char * const arch_timer_list[] = {
	"arm,armv8-timer",
	"arm,armv7-timer",
	NULL
};

static __u32 timer_irq;
static void *dtb;

void generic_timer_mask_irq(void)
{
	set_el0(cntv_ctl, get_el0(cntv_ctl) | GT_TIMER_MASK_IRQ);

	/* Ensure the write of sys register is visible */
	isb();
}

void generic_timer_unmask_irq(void)
{
	set_el0(cntv_ctl, get_el0(cntv_ctl) & ~GT_TIMER_MASK_IRQ);

	/* Ensure the write of sys register is visible */
	isb();
}

__u32 generic_timer_get_frequency(int fdt_timer)
{
	int len;
	const __u64 *fdt_freq;

	/*
	* On a few platforms the frequency is not configured correctly
	* by the firmware. A property in the DT (clock-frequency) has
	* been introduced to workaround those firmware.
	*/
	fdt_freq = fdt_getprop(dtb, fdt_timer, "clock-frequency", &len);
	if (!fdt_freq || (len <= 0)) {
		uk_pr_info("No clock-frequency found, reading from register directly.\n");

		/* No workaround, get from register directly */
		return get_el0(cntfrq);
	}

	return fdt32_to_cpu(fdt_freq[0]);
}

unsigned long sched_have_pending_events;

void time_block_until(__nsec until)
{
	while (ukplat_monotonic_clock() < until) {
		generic_timer_cpu_block_until(until);
		if (__uk_test_and_clear_bit(0, &sched_have_pending_events))
			break;
	}
}

__nsec ukplat_time_get_ticks(void)
{
	return generic_timer_get_ticks();
}

/* must be called before interrupts are enabled */
void ukplat_time_init(void)
{
	int rc, offs;
	struct uk_intctlr_irq irq;

	dtb = (void *)ukplat_bootinfo_get()->dtb;

	/*
	 * Monotonic time begins at boot_ticks (first read of counter
	 * before calibration).
	 */
	generic_timer_update_boot_ticks();

	/* Currently, we only support 1 timer per system */
	offs = fdt_node_offset_by_compatible_list(dtb, -1, arch_timer_list);
	if (unlikely(offs < 0))
		UK_CRASH("Could not find arch timer (%d)\n", offs);

	rc = generic_timer_init(offs);
	if (unlikely(rc < 0))
		UK_CRASH("Failed to initialize platform time (%d)\n", rc);

	rc = uk_intctlr_irq_fdt_xlat(dtb, offs, 2, &irq);
	if (unlikely(rc < 0))
		UK_CRASH("Could not get IRQ from dtb (%d)\n", rc);

	uk_intctlr_irq_configure(&irq);

	rc = uk_intctlr_irq_register(irq.id, generic_timer_irq_handler, NULL);
	if (unlikely(rc < 0))
		UK_CRASH("Failed to register timer interrupt handler\n");

	timer_irq = irq.id;

	/*
	 * Mask IRQ before scheduler start working. Otherwise we will get
	 * unexpected timer interrupts when system is booting.
	 */
	generic_timer_mask_irq();

	/* Enable timer */
	generic_timer_enable();
}

__u32 ukplat_time_get_irq(void)
{
	return timer_irq;
}
