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
/* Taken from solo5 intr.c */

#include <stdlib.h>
#include <uk/alloc.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/plat/irq.h>
#include <uk/plat/common/irq.h>
#include <uk/intctlr.h>
#include <uk/assert.h>
#include <uk/event.h>
#include <uk/trace.h>
#include <uk/print.h>
#include <errno.h>
#include <uk/bitops.h>
#ifdef CONFIG_UKPLAT_ISR_ECTX_ASSERTIONS
#include <uk/arch/ctx.h>
#endif
#include <uk/bitmap.h>

UK_EVENT(UKPLAT_EVENT_IRQ);

UK_TRACEPOINT(trace_plat_kvm_unhandled_irq, "Unhandled irq=%lu\n",
	      unsigned long);

/* IRQ handlers declarations */
struct irq_handler {
	irq_handler_func_t func;
	void *arg;
};

static struct irq_handler irq_handlers[__MAX_IRQ]
				[CONFIG_KVM_MAX_IRQ_HANDLER_ENTRIES];
static unsigned long irqs_allocated[UK_BITS_TO_LONGS(__ALLOCABLE_IRQ_COUNT)];

static inline struct irq_handler *allocate_handler(unsigned long irq)
{
	UK_ASSERT(irq < __MAX_IRQ);
	for (int i = 0; i < CONFIG_KVM_MAX_IRQ_HANDLER_ENTRIES; i++)
		if (irq_handlers[irq][i].func == NULL)
			return &irq_handlers[irq][i];
	return NULL;
}

int ukplat_irq_register(unsigned long irq, irq_handler_func_t func, void *arg)
{
	struct irq_handler *h;
	unsigned long flags;

	UK_ASSERT(func);
	if (irq >= __MAX_IRQ)
		return -EINVAL;

	flags = ukplat_lcpu_save_irqf();
	h = allocate_handler(irq);
	if (!h) {
		ukplat_lcpu_restore_irqf(flags);
		return -ENOMEM;
	}

	h->func = func;
	h->arg = arg;

	ukplat_lcpu_restore_irqf(flags);

	intctrl_clear_irq(irq);
	return 0;
}

int ukplat_irq_unregister(unsigned long irq, irq_handler_func_t func)
{
	struct irq_handler *h = NULL;
	unsigned long flags;
	int i;

	UK_ASSERT(func);
	if (irq >= __MAX_IRQ)
		return -EINVAL;

	flags = ukplat_lcpu_save_irqf();

	for (i = 0; i < CONFIG_KVM_MAX_IRQ_HANDLER_ENTRIES; i++) {
		if (irq_handlers[irq][i].func == func) {
			h = &irq_handlers[irq][i];
			break;
		}
	}
	if (h == NULL) {
		ukplat_lcpu_restore_irqf(flags);
		uk_pr_crit(
			"attempt to unregister irq handler %p for irq %lu "
			"which wasn't registered previously", func, irq);
		return -ENOENT;
	}

	h->func = NULL;
	h->arg = NULL;

	ukplat_lcpu_restore_irqf(flags);
	return 0;
}

int ukplat_irq_alloc(unsigned int *irqs, __u16 count)
{
	unsigned long start, idx;

	start = uk_bitmap_find_next_zero_area(irqs_allocated,
					      __ALLOCABLE_IRQ_COUNT, 0, count,
					      0);
	if (start == __ALLOCABLE_IRQ_COUNT)
		return -ENOSPC;

	uk_bitmap_set(irqs_allocated, start, count);
	for (idx = start; idx < (start + count); idx++) {
		*irqs = idx + __FIRST_ALLOCABLE_IRQ;
		irqs++;
	}
	return 0;
}

int ukplat_irq_free(unsigned int *irqs, __u16 count)
{
	unsigned long i;
	for (i = 0; i < count; i++) {
		UK_ASSERT(__FIRST_ALLOCABLE_IRQ <= irqs[i] &&
			  irqs[i] <= __LAST_ALLOCABLE_IRQ);
		if (!uk_test_and_clear_bit(irqs[i] - __FIRST_ALLOCABLE_IRQ,
					   irqs_allocated)) {
			UK_CRASH("attempted to free unallocated IRQ\n");
		}
	}

	return 0;
}

/*
 * TODO: This is a temporary solution used to identify non TSC clock
 * interrupts in order to stop waiting for interrupts with deadline.
 */
extern unsigned long sched_have_pending_events;

void _ukplat_irq_handle(struct __regs *regs, unsigned long irq)
{
	struct irq_handler *h;
	int i;
	int rc;
	struct ukplat_event_irq_data ctx;
#ifdef CONFIG_UKPLAT_ISR_ECTX_ASSERTIONS
	__sz ectx_align = ukarch_ectx_align();
	__u8 ectxbuf[ukarch_ectx_size() + ectx_align];
	struct ukarch_ectx *ectx = (struct ukarch_ectx *)
		ALIGN_UP((__uptr) ectxbuf, ectx_align);

	ukarch_ectx_init(ectx);
#endif

	UK_ASSERT(irq < __MAX_IRQ);

	ctx.regs = regs;
	ctx.irq = irq;
	rc = uk_raise_event(UKPLAT_EVENT_IRQ, &ctx);
	if (unlikely(rc < 0))
		UK_CRASH("IRQ event handler returned error: %d\n", rc);
	if (rc == UK_EVENT_HANDLED) {
		/* Skip all normal handlers if an event handler handled the
		 * event
		 */
		goto exit_ack;
	}

	for (i = 0; i < CONFIG_KVM_MAX_IRQ_HANDLER_ENTRIES; i++) {
		if (irq_handlers[irq][i].func == NULL)
			break;
		h = &irq_handlers[irq][i];
		if (irq != ukplat_time_get_irq())
			/* ukplat_time_get_irq() gives the IRQ reserved for a timer,
			 * responsible to wake up cpu from halt, so it can check if
			 * it has something to do. Effectively it is OS ticks.
			 *
			 * If interrupt comes not from the timer, the
			 * chances are some work have just
			 * arrived. Let's kick the scheduler out of
			 * the halting loop, and let it take care of
			 * that work.
			 */
			__uk_test_and_set_bit(0, &sched_have_pending_events);

		if (h->func(h->arg) == 1) {
			/* APIC interrupts are currently ack'ed in the handler */
			if (irq >= __FIRST_ALLOCABLE_IRQ)
				return;

			goto exit_ack;
		}
	}
	/*
	 * Acknowledge interrupts even in the case when there was no handler for
	 * it. We do this to (1) compensate potential spurious interrupts of
	 * devices, and (2) to minimize impact on drivers that share one
	 * interrupt line that would then stay disabled.
	 */
	trace_plat_kvm_unhandled_irq(irq);

exit_ack:
#ifdef CONFIG_UKPLAT_ISR_ECTX_ASSERTIONS
	ukarch_ectx_assert_equal(ectx);
#endif
	intctrl_ack_irq(irq);
}

int ukplat_irq_init(struct uk_alloc *a __unused)
{
	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	/* Nothing for now */
	return 0;
}
