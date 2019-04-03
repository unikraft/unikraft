/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
#include <string.h>
#include <uk/alloc.h>
#include <uk/list.h>
#include <uk/plat/lcpu.h>
#include <uk/plat/irq.h>
#include <uk/assert.h>
#include <linuxu/syscall.h>
#include <linuxu/signal.h>

#define IRQS_NUM    16

/* IRQ handlers declarations */
struct irq_handler {
	irq_handler_func_t func;
	void *arg;

	struct uk_sigaction oldaction;

	UK_SLIST_ENTRY(struct irq_handler) entries;
};

UK_SLIST_HEAD(irq_handler_head, struct irq_handler);
static struct irq_handler_head irq_handlers[IRQS_NUM];

static struct uk_alloc *allocator;
static k_sigset_t handled_signals_set;
static unsigned long irq_enabled;

void ukplat_lcpu_enable_irq(void)
{
	int rc;

	rc = sys_sigprocmask(SIG_UNBLOCK, &handled_signals_set, NULL);
	if (unlikely(rc != 0))
		UK_CRASH("Failed to unblock signals (%d)\n", rc);

	irq_enabled = 1;
}

void ukplat_lcpu_disable_irq(void)
{
	int rc;

	rc = sys_sigprocmask(SIG_BLOCK, &handled_signals_set, NULL);
	if (unlikely(rc != 0))
		UK_CRASH("Failed to block signals (%d)\n", rc);

	irq_enabled = 0;
}

int ukplat_lcpu_irqs_disabled(void)
{
	return (irq_enabled == 0);
}

unsigned long ukplat_lcpu_save_irqf(void)
{
	unsigned long flags = irq_enabled;

	if (irq_enabled)
		ukplat_lcpu_disable_irq();

	return flags;
}

void ukplat_lcpu_restore_irqf(unsigned long flags)
{
	if (flags) {
		if (!irq_enabled)
			ukplat_lcpu_enable_irq();

	} else if (irq_enabled)
		ukplat_lcpu_disable_irq();
}

void ukplat_lcpu_irqs_handle_pending(void)
{
	/* TO BE DONE */
}

void __restorer(void);
#if defined __X86_64__
asm("__restorer:mov $15,%rax\nsyscall");
#elif defined __ARM_32__
asm("__restorer:mov r7, #0x77\nsvc 0x0");
#else
#error "Unsupported architecture"
#endif

static void _irq_handle(int irq)
{
	struct irq_handler *h;

	UK_ASSERT(irq >= 0 && irq < IRQS_NUM);

	UK_SLIST_FOREACH(h, &irq_handlers[irq], entries) {
		if (h->func(h->arg) == 1)
			return;
	}
	/*
	 * Just warn about unhandled interrupts. We do this to
	 * (1) compensate potential spurious interrupts of
	 * devices, and (2) to minimize impact on drivers that share
	 * one interrupt line that would then stay disabled.
	 */
	uk_pr_crit("Unhandled irq=%d\n", irq);
}

int ukplat_irq_register(unsigned long irq, irq_handler_func_t func, void *arg)
{
	struct irq_handler *h;
	struct uk_sigaction action;
	k_sigset_t set;
	unsigned long flags;
	int rc;

	if (irq >= IRQS_NUM)
		return -EINVAL;

	/* New handler */
	h = uk_malloc(allocator, sizeof(struct irq_handler));
	if (!h)
		return -ENOMEM;
	h->func = func;
	h->arg = arg;

	/* Register signal action */
	memset(&action, 0, sizeof(action));
	action.k_sa_handler = _irq_handle;
	action.k_sa_flags = SA_RESTORER;
	action.k_sa_restorer = __restorer;

	rc = sys_sigaction((int) irq, &action, &h->oldaction);
	if (rc != 0)
		goto err;

	flags = ukplat_lcpu_save_irqf();
	UK_SLIST_INSERT_HEAD(&irq_handlers[irq], h, entries);
	ukplat_lcpu_restore_irqf(flags);

	/* Unblock the signal */
	k_sigemptyset(&set);
	k_sigaddset(&set, irq);

	rc = sys_sigprocmask(SIG_UNBLOCK, &set, NULL);
	if (unlikely(rc != 0))
		UK_CRASH("Failed to unblock signals: %d\n", rc);

	/* Add to our handled signals set */
	k_sigaddset(&handled_signals_set, irq);

	return 0;

err:
	uk_free(allocator, h);
	return -rc;
}

int ukplat_irq_init(struct uk_alloc *a)
{
	UK_ASSERT(!irq_enabled);
	UK_ASSERT(!allocator);

	allocator = a;

	/* Clear list head */
	for (int i = 0; i < IRQS_NUM; i++)
		UK_SLIST_INIT(&irq_handlers[i]);

	k_sigemptyset(&handled_signals_set);

	return 0;
}
