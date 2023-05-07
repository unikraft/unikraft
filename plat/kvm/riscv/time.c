/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Eduard Vintila <eduard.vintila47@gmail.com>
 *
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
#include <libfdt.h>
#include <uk/plat/time.h>
#include <uk/plat/lcpu.h>
#include <uk/bitops.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/irq.h>
#include <riscv/sbi.h>
#include <riscv/time.h>
#include <kvm/config.h>
#include <uk/assert.h>
#include <rtc/goldfish.h>

int timer_irq_handler(void *arg __unused)
{
	return 1;
}

__nsec ukplat_monotonic_clock(void)
{
	return timer_monotonic_clock();
}

__nsec ukplat_wall_clock(void)
{
	return ukplat_monotonic_clock() + timer_epoch_offset();
}

void ukplat_time_init(void)
{
	int rc;

	rc = goldfish_init_rtc(_libkvmplat_cfg.dtb);
	if (rc < 0)
		uk_pr_warn(
		    "RTC device not found, wall time will not be accurate\n");

	rc = init_timer(_libkvmplat_cfg.dtb);
	if (rc < 0)
		UK_CRASH("Could not initialize the RISC-V timer\n");

	rc = ukplat_irq_register(0, timer_irq_handler, NULL);
	if (rc < 0)
		UK_CRASH("Could not register the timer interrupt handler\n");
}

void ukplat_time_fini(void) {}

uint32_t ukplat_time_get_irq(void)
{
	return 0;
}

unsigned long sched_have_pending_events;

void time_block_until(__nsec until)
{
	while (ukplat_monotonic_clock() < until) {
		timer_cpu_block_until(until);
		if (__uk_test_and_clear_bit(0, &sched_have_pending_events))
			break;
	}
}
