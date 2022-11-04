/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2009 Citrix Systems, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <xen-arm/os.h>
#include <uk/plat/common/irq.h>
#include <common/events.h>
#include <xen-arm/traps.h>
#include <uk/print.h>
#include <uk/assert.h>
#include <uk/arch/types.h>
#include <uk/plat/time.h>

/************************************************************************
 * Time functions
 *************************************************************************/

static uint64_t cntvct_at_init;
static uint32_t counter_freq;
/*
 * System Time
 * 64 bit value containing the nanoseconds elapsed since boot time.
 * This value is adjusted by frequency drift.
 * NOW() returns the current time.
 * The other macros are for convenience to approximate short intervals
 * of real time into system time
 */
typedef int64_t s_time_t;

#define SECONDS(_s)   (((s_time_t)(_s))  * 1000000000UL)

/* Compute with 96 bit intermediate result: (a*b)/c */
uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c)
{
	union {
		uint64_t ll;
		struct {
			uint32_t low, high;
		} l;
	} u, res;
	uint64_t rl, rh;

	u.ll = a;
	rl = (uint64_t)u.l.low * (uint64_t)b;
	rh = (uint64_t)u.l.high * (uint64_t)b;
	rh += (rl >> 32);
	res.l.high = rh / c;
	res.l.low = (((rh % c) << 32) + (rl & 0xffffffff)) / c;
	return res.ll;
}

static inline s_time_t ticks_to_ns(uint64_t ticks)
{
	return muldiv64(ticks, SECONDS(1), counter_freq);
}

static inline uint64_t ns_to_ticks(s_time_t ns)
{
	return muldiv64(ns, counter_freq, SECONDS(1));
}

static inline uint64_t read_virtual_count(void)
{
#if defined(__arm__)
	uint32_t c_lo, c_hi;

	__asm__ __volatile__("mrrc p15, 1, %0, %1, c14"
			     : "=r"(c_lo), "=r"(c_hi));
	return (((uint64_t) c_hi) << 32) + c_lo;
#elif defined(__aarch64__)
	uint64_t c;

	isb();
	__asm__ __volatile__("mrs %0, cntvct_el0":"=r"(c));
	return c;
#endif

}

/* monotonic_clock(): returns # of nanoseconds passed since time_init()
 *        Note: This function is required to return accurate
 *        time even in the absence of multiple timer ticks.
 */
__nsec ukplat_monotonic_clock(void)
{
	return (__nsec) ticks_to_ns(read_virtual_count() - cntvct_at_init);
}

/* TODO: proper implementation of wall clock time */
__nsec ukplat_wall_clock(void)
{
	return ukplat_monotonic_clock();
}

/* Set the timer and mask. */
void write_timer_ctl(uint32_t value)
{
#if defined(__arm__)
	__asm__ __volatile__("mcr p15, 0, %0, c14, c3, 1\n"
			     "isb"::"r"(value));
#elif defined(__aarch64__)
	__asm__ __volatile__("msr cntv_ctl_el0, %0" : : "r" (value));
	isb();
#endif

}

void set_vtimer_compare(uint64_t value)
{
	uk_pr_debug("New CompareValue : %llx\n", value);

#if defined(__arm__)
	__asm__ __volatile__("mcrr p15, 3, %0, %H0, c14"
			     ::"r"(value));
#elif defined(__aarch64__)
#endif

	/* Enable timer and unmask the output signal */
	write_timer_ctl(1);
}

void unset_vtimer_compare(void)
{
	/* Disable timer and mask the output signal */
	write_timer_ctl(2);
}

void block_domain(__snsec until)
{
	uint64_t until_count = ns_to_ticks(until) + cntvct_at_init;

	UK_ASSERT(irqs_disabled());
	if (read_virtual_count() < until_count) {
		set_vtimer_compare(until_count);
		__asm__ __volatile__("wfi");
		unset_vtimer_compare();

		/* Give the IRQ handler a chance to handle
		 * whatever woke us up.
		 */
		local_irq_enable();
		local_irq_disable();
	}
}

void time_block_until(__snsec until)
{
	block_domain(until);
}

void ukplat_time_init(void)
{
	uk_pr_info("Initialising timer interface\n");

#if defined(__arm__)
	__asm__ __volatile__("mrc p15, 0, %0, c14, c0, 0":"=r"(counter_freq));
#elif defined(__aarch64__)
	__asm__ __volatile__("mrs %0, cntfrq_el0":"=r"(counter_freq));
#endif

	cntvct_at_init = read_virtual_count();
	uk_pr_debug("Virtual Count register is %llx, freq = %d Hz\n",
		    cntvct_at_init, counter_freq);
}

void ukplat_time_fini(void)
{
}
