/* SPDX-License-Identifier: MIT */
/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 * (C) 2002-2003 - Keir Fraser - University of Cambridge
 * (C) 2005 - Grzegorz Milos - Intel Research Cambridge
 * (C) 2006 - Robert Kaiser - FH Wiesbaden
 ****************************************************************************
 *
 *        File: time.c
 *      Author: Rolf Neugebauer and Keir Fraser
 *     Changes: Grzegorz Milos
 *
 * Description: Simple time and timer functions
 *              Ported from Mini-OS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <uk/plat/time.h>
#include <x86/cpu.h>
#include <_time.h>
#include <common/hypervisor.h>
#include <common/events.h>
#include <xen-x86/irq.h>
#include <uk/assert.h>

/************************************************************************
 * Time functions
 *************************************************************************/

/* These are peridically updated in shared_info, and then copied here. */
struct shadow_time_info {
	uint64_t tsc_timestamp;     /* TSC at last update of time vals.  */
	uint64_t system_timestamp;  /* Time, in nanosecs, since boot.    */
	uint32_t tsc_to_nsec_mul;
	uint32_t tsc_to_usec_mul;
	int tsc_shift;
	uint32_t version;
};

static struct timespec shadow_ts;

static uint32_t shadow_ts_version;

static struct shadow_time_info shadow;


/* TODO move this where it belongs: */
#ifndef rmb
#define rmb()  __asm__ __volatile__("lock; addl $0,0(%%esp)" : : : "memory")
#endif

#define HANDLE_USEC_OVERFLOW(_tv)			\
	do {						\
		while ((_tv)->tv_usec >= 1000000) {	\
			(_tv)->tv_usec -= 1000000;	\
			(_tv)->tv_sec++;		\
		}					\
	} while (0)

static inline int time_values_up_to_date(void)
{
	struct vcpu_time_info *src = &HYPERVISOR_shared_info->vcpu_info[0].time;

	return (shadow.version == src->version);
}

static inline int wc_values_up_to_date(void)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	return (shadow_ts_version == s->wc_version);
}

/*
 * Scale a 64-bit delta by scaling and multiplying by a 32-bit fraction,
 * yielding a 64-bit result.
 */
static inline uint64_t scale_delta(uint64_t delta, uint32_t mul_frac, int shift)
{
	uint64_t product;
#ifdef __i386__
	uint32_t tmp1, tmp2;
#endif

	if (shift < 0)
		delta >>= -shift;
	else
		delta <<= shift;

#ifdef __i386__
	__asm__(
		"mul  %5       ; "
		"mov  %4,%%eax ; "
		"mov  %%edx,%4 ; "
		"mul  %5       ; "
		"add  %4,%%eax ; "
		"xor  %5,%5    ; "
		"adc  %5,%%edx ; "
		: "=A" (product), "=r" (tmp1), "=r" (tmp2)
		: "a" ((uint32_t)delta), "1" ((uint32_t)(delta >> 32)), "2" (mul_frac)
	);
#else
	__asm__(
		"mul %%rdx ; shrd $32,%%rdx,%%rax"
		: "=a" (product) : "0" (delta), "d" ((uint64_t)mul_frac)
	);
#endif

	return product;
}


static unsigned long get_nsec_offset(void)
{
	uint64_t now, delta;

	now = rdtsc();
	delta = now - shadow.tsc_timestamp;

	return scale_delta(delta, shadow.tsc_to_nsec_mul, shadow.tsc_shift);
}


static void get_time_values_from_xen(void)
{
	struct vcpu_time_info *src = &HYPERVISOR_shared_info->vcpu_info[0].time;

	do {
		shadow.version = src->version;
		rmb();
		shadow.tsc_timestamp     = src->tsc_timestamp;
		shadow.system_timestamp  = src->system_time;
		shadow.tsc_to_nsec_mul   = src->tsc_to_system_mul;
		shadow.tsc_shift         = src->tsc_shift;
		rmb();
	} while ((src->version & 1) | (shadow.version ^ src->version));

	shadow.tsc_to_usec_mul = shadow.tsc_to_nsec_mul / 1000;
}




/* monotonic_clock(): returns # of nanoseconds passed since time_init()
 *		Note: This function is required to return accurate
 *		time even in the absence of multiple timer ticks.
 */
__nsec ukplat_monotonic_clock(void)
{
	uint64_t time;
	uint32_t local_time_version;

	do {
		local_time_version = shadow.version;
		rmb();
		time = shadow.system_timestamp + get_nsec_offset();
		if (!time_values_up_to_date())
			get_time_values_from_xen();
		rmb();
	} while (local_time_version != shadow.version);

	return time;
}

static void update_wallclock(void)
{
	shared_info_t *s = HYPERVISOR_shared_info;

	do {
		shadow_ts_version = s->wc_version;
		rmb();
		shadow_ts.tv_sec  = s->wc_sec;
		shadow_ts.tv_nsec = s->wc_nsec;
		rmb();
	} while ((s->wc_version & 1) | (shadow_ts_version ^ s->wc_version));
}

__nsec ukplat_wall_clock(void)
{
	__nsec ret = ukplat_monotonic_clock();

	if (!wc_values_up_to_date())
		update_wallclock();

	ret += ukarch_time_sec_to_nsec((__nsec) shadow_ts.tv_sec);
	ret += (__nsec) shadow_ts.tv_nsec;

	return ret;
}

void time_block_until(__snsec until)
{
	UK_ASSERT(irqs_disabled());

	if ((__snsec) ukplat_monotonic_clock() < until) {
		HYPERVISOR_set_timer_op(until);
		ukplat_lcpu_halt_irq();
		HYPERVISOR_set_timer_op(0);
	}
}

static void timer_handler(evtchn_port_t ev __unused,
		struct __regs *regs __unused, void *ign __unused)
{
	__nsec until = ukplat_monotonic_clock() + UKPLAT_TIME_TICK_NSEC;

	HYPERVISOR_set_timer_op(until);
}



static evtchn_port_t port;
void ukplat_time_init(void)
{
	uk_pr_debug("Initializing timer interface\n");
	port = bind_virq(VIRQ_TIMER, &timer_handler, NULL);
	unmask_evtchn(port);
}

void ukplat_time_fini(void)
{
	/* Clear any pending timer */
	HYPERVISOR_set_timer_op(0);
	unbind_evtchn(port);
}

#ifdef CONFIG_MIGRATION /* TODO wip */
void suspend_time(void)
{
	/* Clear any pending timer */
	HYPERVISOR_set_timer_op(0);
	unbind_evtchn(port);
}

void resume_time(void)
{
	port = bind_virq(VIRQ_TIMER, &timer_handler, NULL);
	unmask_evtchn(port);
}
#endif
