/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <uk/clock_event.h>
#include <uk/intctlr.h>
#include <uk/intctlr/apic.h>
#include <uk/init.h>

#include <kvm/tscclock.h>

#include <x86/cpu.h>
#include <uk/asm/apic.h>

static int apic_timer_disable(struct uk_clock_event *ce);
static int apic_timer_set_next_event(struct uk_clock_event *ce, __nsec at);

struct apic_configuration {
	/* TODO: Implement support for LAPICs without deadline TSC support
	 * int deadline;
	 */
};

static struct apic_configuration apic_configuration;

static struct uk_clock_event apic_clock_event = {
	.name = "lapic",
	.priority = 100,
	.set_next_event = apic_timer_set_next_event,
	.disable = apic_timer_disable,
	.priv = &apic_configuration
};

static int apic_timer_set_next_event(struct uk_clock_event *ce __unused,
                                     __nsec at)
{
	__nsec now = ukplat_monotonic_clock();
	__nsec delta = at > now ? at - now : 0;

	wrmsrl(APIC_MSR_TSC_DEADLINE,
	       rdtsc() + tscclock_ns_to_tsc_delta(delta));

	return 0;
}

static int apic_timer_disable(struct uk_clock_event *ce __unused)
{
	/* Writing a zero will deactivate the timer */
	wrmsr(APIC_MSR_TSC_DEADLINE, 0, 0);

	return 0;
}

static int apic_timer_irq_handler(void *argp)
{
	struct uk_clock_event *ce = argp;

	if (ce->event_handler)
		ce->event_handler(ce);

	return 1;
}

static int register_apic_clock_event(struct uk_init_ctx *ctx __unused)
{
	__u32 eax, ebx, ecx, edx, irq;
	int deadline, rc;

	/* We prefer the TSC deadline mode because we have a common time base
	 * and frequency. Otherwise, we have to figure out the frequency of the
	 * APIC timer.
	 */
	cpuid(1, 0, &eax, &ebx, &ecx, &edx);
	deadline = (ecx & X86_CPUID1_ECX_TSC_DEADLINE) != 0;
	if (!deadline) {
		uk_pr_warn("LAPIC does not support TSC deadline mode\n");
		return 0;
	}

	/* Allocate and set up a new interrupt for the timer. */
	rc = uk_intctlr_irq_alloc(&irq, 1);
	if (unlikely(rc))
		return rc;

	rc = uk_intctlr_irq_register(irq, apic_timer_irq_handler,
				 &apic_clock_event);
	if (unlikely(rc))
		goto err_irq;

	apic_set_timer(APIC_TIMER_MODE_TSC_DEADLINE, 32 + irq);

	apic_timer_disable(&apic_clock_event);
	apic_set_timer_mask(0);

	uk_clock_event_register(&apic_clock_event);

out:
	return rc;
err_irq:
	uk_intctlr_irq_free(&irq, 1);
	goto out;
}

uk_early_initcall_prio(register_apic_clock_event, 0, 0);
