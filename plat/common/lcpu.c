/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Marc Rittinghaus <marc.rittinghaus@kit.edu>
 *          Răzvan Vîrtan <virtanrazvan@gmail.com>
 *          Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University Politehnica of Bucharest.
 *                     All rights reserved.
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

#include <uk/essentials.h>
#include <uk/arch/atomic.h>
#if CONFIG_HAVE_SMP
#include <uk/intctlr.h>
#endif /* CONFIG_HAVE_SMP */
#include <uk/plat/lcpu.h>
#include <uk/plat/time.h>
#include <uk/plat/common/cpu.h>
#include <uk/plat/common/lcpu.h>
#include <uk/plat/common/_time.h>
#include <uk/print.h>

#include <limits.h>
#include <errno.h>

/**
 * Array of LCPUs, one for every CPU in the system.
 *
 * TODO: Preferably have a more flexible solution that does not waste memory
 * for non-present CPUs and does not force us to configure the maximum number
 * of CPUs beforehand.
 */
UKPLAT_PER_LCPU_DEFINE(struct lcpu, lcpus);

#ifdef CONFIG_HAVE_SMP
/**
 * Number of allocated logical CPUs in the system.
 * Must be [1, CONFIG_UKPLAT_LCPU_MAXCOUNT) after boot
 */
static __u32 lcpu_count = 1;

struct lcpu *lcpu_alloc(__lcpuid id)
{
	struct lcpu *lcpu;

	if (lcpu_count == CONFIG_UKPLAT_LCPU_MAXCOUNT)
		return NULL;

	lcpu = &lcpus[lcpu_count];
	lcpu->state = LCPU_STATE_OFFLINE;
	lcpu->id    = id;
	lcpu->idx   = lcpu_count;

	lcpu_count++;

	return lcpu;
}

__u32 ukplat_lcpu_count(void)
{
	return lcpu_count;
}
#else
#define lcpu_count	(1)

/* Provide weak default implementations for the case that the architecture does
 * not support multi-processor configurations.
 */
__lcpuid __weak lcpu_arch_id(void)
{
	return 0;
}

int __weak lcpu_arch_init(struct lcpu *this_lcpu __unused)
{
	return 0;
}
#endif /* !CONFIG_HAVE_SMP */

struct lcpu *lcpu_get(__lcpuidx idx)
{
	UK_ASSERT(idx < lcpu_count);

	return &lcpus[idx];
}

int lcpu_init(struct lcpu *this_lcpu)
{
	int rc;

	/*
	 * NOTE: Do not use anything that might need initialized exception
	 * traps until after lcpu_arch_init(), as traps might not be
	 * initialized for this CPU yet!
	 */

	/* Initialize the bootstrap CPU */
	if (lcpu_is_bsp(this_lcpu)) {
		if (unlikely(lcpu_count > 1))
			return -EPERM;

		this_lcpu->idx   = 0;
		this_lcpu->id    = lcpu_arch_id();
		this_lcpu->state = LCPU_STATE_INIT;
	} else {
		/* We should already be in INIT state for secondary CPUs */
		UK_ASSERT(this_lcpu->state == LCPU_STATE_INIT);
	}

	/* Do architecture-specific initialization */
	rc = lcpu_arch_init(this_lcpu);
	if (unlikely(rc))
		return rc;

	UK_ASSERT(ukplat_lcpu_irqs_disabled());

#ifdef CONFIG_HAVE_SMP
	this_lcpu->fn.fn = NULL;
#endif /* CONFIG_HAVE_SMP */

	/* Write back changes before marking CPU as online */
	wmb();

	/* Put the CPU in busy state. This will mark it as online. After this
	 * point, functions may be queued to the CPU. However, IRQs are still
	 * disabled.
	 */
	this_lcpu->state = LCPU_STATE_BUSY0;

	return 0;
}

static void __noreturn lcpu_halt(struct lcpu *this_cpu, int error_code)
{
	ukplat_lcpu_disable_irq();

	this_cpu->state = LCPU_STATE_HALTED;
	this_cpu->error_code = error_code;

	while (1) {
		/* Although we should not be able to recover via regular
		 * interrupts, we might receive NMIs so loop to be safe.
		 */
		halt();
	}
}

void __noreturn ukplat_lcpu_halt(void)
{
	lcpu_halt(lcpu_get_current(), 0);
}

void ukplat_lcpu_halt_irq_until(__nsec until)
{
	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	time_block_until(until);
}

#ifdef CONFIG_HAVE_SMP
int lcpu_fn_enqueue(struct lcpu *lcpu, const struct ukplat_lcpu_func *fn)
{
	void (*old_fn)(struct __regs *, void *);

	UK_ASSERT(fn->fn);

	old_fn = ukarch_load_n(&lcpu->fn.fn);

	/* Check if the slot is empty */
	if (old_fn != NULL)
		return -EAGAIN;

	/* It is empty, try to store the function */
	if (ukarch_compare_exchange_sync(&lcpu->fn.fn, old_fn,
					 fn->fn) != fn->fn)
		return -EAGAIN;

	/* We have acquired the slot! Also store the user argument.
	 * It is safe to do it afterwards, because the RUN IRQ handler will
	 * only take one function and return afterwards. And we only raise the
	 * IRQ after finishing setup.
	 */
	lcpu->fn.user = fn->user;

	/* Ensure everything is written back when we return and the arch
	 * support code will raise the IRQ
	 */
	wmb();

	return 0;
}

static void lcpu_fn_dequeue(struct lcpu *this_lcpu, struct ukplat_lcpu_func *fn)
{
	*fn = this_lcpu->fn;

	/* Ensure that we have captured the whole function object */
	rmb();

	UK_ASSERT(fn->fn);

	/* Free the slot. Another function object can be queued afterwards */
	this_lcpu->fn.fn = NULL;
}

static int lcpu_ipi_run_handler(void *args __unused)
{
	struct lcpu *this_lcpu = lcpu_get_current();
	struct ukplat_lcpu_func fn;

	lcpu_fn_dequeue(this_lcpu, &fn);

	/* TODO: Provide the register snapshot from the trap frame */
	fn.fn(NULL, fn.user);

	/* If we had a transition from BUSY to HALTED in fn, we would
	 * not reach this code but sit in the error halt loop. We can
	 * thus safely just decrement without worrying about the HALTED
	 * state.
	 */
	UK_ASSERT(lcpu_state_is_busy(this_lcpu->state));
	ukarch_dec(&this_lcpu->state);

	return 1;
}

static int lcpu_ipi_wakeup_handler(void *args __unused)
{
	/* Nothing to do */
	return 1;
}

/* We want these to be externally defined as const to clarify that the vectors
 * cannot be changed after initialization. However, we still need them non-const
 * so we can still set them here. While we can do a DECONST and force allocation
 * in .bss, we enter undefined behavior territory. So we just export a const
 * pointer as proxy. This is still faster than calling a getter function and
 * with LTO this will be optimized to a direct access.
 */
static unsigned long _lcpu_run_irqv;
static unsigned long _lcpu_wakeup_irqv;

const unsigned long * const lcpu_run_irqv = &_lcpu_run_irqv;
const unsigned long * const lcpu_wakeup_irqv = &_lcpu_wakeup_irqv;

int lcpu_mp_init(unsigned long run_irq, unsigned long wakeup_irq, void *arg)
{
	int rc;

	/* Make sure this is run on the BSP only */
	UK_ASSERT(lcpu_count == 1);
	UK_ASSERT(lcpu_current_is_bsp());

	/* Initialize architecture-dependent functionality. This will also do
	 * CPU discovery and allocation
	 */
	rc = lcpu_arch_mp_init(arg);
	if (unlikely(rc))
		return rc;

	/* Register the lcpu_run and lcpu_wakeup interrupt handlers */
	rc = uk_intctlr_irq_register(run_irq, lcpu_ipi_run_handler, NULL);
	if (unlikely(rc)) {
		uk_pr_crit("Could not register handler for IPI IRQ %ld\n",
			   run_irq);
		return rc;
	}

	rc = uk_intctlr_irq_register(wakeup_irq, lcpu_ipi_wakeup_handler, NULL);
	if (unlikely(rc)) {
		uk_pr_crit("Could not register handler for wakeup IRQ %ld\n",
			   wakeup_irq);
		return rc;
	}

	_lcpu_run_irqv = run_irq;
	_lcpu_wakeup_irqv = wakeup_irq;

	return 0;
}

void __weak __noreturn lcpu_entry_default(struct lcpu *this_lcpu)
{
	struct lcpu_sargs s_args = this_lcpu->s_args;
	int rc;

	UK_ASSERT(!lcpu_is_bsp(this_lcpu));

	/* Finish initialization. As there is nothing to return to, we
	 * just enter halted state if an error occurs.
	 */
	rc = lcpu_init(this_lcpu);
	if (unlikely(rc))
		lcpu_halt(this_lcpu, rc);

	/* If the user supplied an entry function jump to it */
	if (s_args.entry && s_args.entry != lcpu_entry_default) {
		/* Does not return */
		lcpu_arch_jump_to(s_args.stackp, s_args.entry);
	} else {
		/* We are coming from BUSY0 state and want to transition to
		 * IDLE state. However, there can be functions queued already
		 * so we have to use a decrement here.
		 */
		ukarch_dec(&this_lcpu->state);

		/* Enable IRQs. If there are functions queued we will
		 * immediately jump to the IRQ handler.
		 */
		ukplat_lcpu_enable_irq();
		while (1) {
			/* Besides interrupts in general, the halt can be
			 * interrupted by calls to ukplat_lcpu_run().
			 */
			halt();
		}
	}
}

int ukplat_lcpu_start(const __lcpuidx lcpuidx[], unsigned int *num, void *sp[],
		      const ukplat_lcpu_entry_t entry[], unsigned long flags)
{
	__lcpuid this_cpu_id = ukplat_lcpu_id();
	struct lcpu *lcpu;
	unsigned int i, n, argi = 0;
	const int new = LCPU_STATE_INIT;
	int old;
	int rc = 0;
#ifdef LCPU_ARCH_MULTI_PHASE_STARTUP
	int rc2;
#endif /* LCPU_ARCH_MULTI_PHASE_STARTUP */

	UK_ASSERT(((lcpuidx) && (num)) || ((!lcpuidx) && (!num)));
	UK_ASSERT(sp);

	lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id) {
			/* If the caller did not supply an index array, we
			 * assume that we do not have parameters and a stack
			 * for the executing CPU. Otherwise, i.e., if the
			 * caller explicitly put the executing CPU in, we still
			 * ignore it but need to skip the parameters.
			 */
			if (lcpuidx)
				argi++;

			continue;
		}

retry:
		old = ukarch_load_n(&lcpu->state);

		/* We ignore CPUs that are already started */
		if (unlikely(old != LCPU_STATE_OFFLINE)) {
			uk_pr_warn("Failed to start CPU 0x%lx: not offline\n",
				   lcpu->id);

			/* Skip CPU and its arguments*/
			argi++;
			continue;
		}

		/* Try to acquire the CPU for initialization. If another thread
		 * was faster, we will return to the state comparison and
		 * report that the CPU is not offline.
		 */
		if (ukarch_compare_exchange_sync((int *)&lcpu->state, old,
						 new) != new)
			goto retry;

		UK_ASSERT(lcpu->state == LCPU_STATE_INIT);

		/* Setup startup arguments.
		 * Since we are ignoring the executing CPU, we must keep a
		 * separate counter to index the arguments.
		 */
		lcpu->s_args.entry = (entry && entry[argi]) ?
			entry[argi] : lcpu_entry_default;
		lcpu->s_args.stackp = sp[argi];

		/* Ensure that the startup arguments have been written back
		 * before issuing the startup call
		 */
		wmb();

		rc = lcpu_arch_start(lcpu, flags);
		if (unlikely(rc)) {
			lcpu->state = LCPU_STATE_HALTED;
			lcpu->error_code = rc;

			/* There is a serious problem. Stop here. The caller
			 * can skip the CPU by using the value of *num.
			 */
			break;
		}

		/* Move to next argument */
		argi++;
	}

#ifdef LCPU_ARCH_MULTI_PHASE_STARTUP
	/* At this point, i has been set to the number of successfully
	 * started CPUs. So if there has been an error, we won't touch
	 * any CPUs not started.
	 */
	rc2 = lcpu_arch_post_start(lcpuidx, &i);
	if (unlikely(rc2)) {
		if (num) {
			UK_ASSERT(i <= *num);
			*num = i;
		}

		/* Return the first error */
		return (rc) ? rc : rc2;
	}
#endif /* LCPU_ARCH_MULTI_PHASE_STARTUP */

	UK_ASSERT(num == NULL || *num == i);
	return rc;
}

static inline int lcpu_transition_safe(struct lcpu *lcpu, int incr)
{
	int old, new;

	/* Transition the CPU to a different busy level. The CPU could not be
	 * online or fall into a halted state at any moment, we thus cannot
	 * just atomically in-/decrement the state. Otherwise, we might corrupt
	 * the non-online state.
	 */
	do {
		old = ukarch_load_n(&lcpu->state);

		/* We must not change the state if the CPU is not online */
		if (!lcpu_state_is_online(old))
			return 0;

		UK_ASSERT(old <= INT_MAX - incr);
		UK_ASSERT(old >= INT_MIN + incr);
		new = old + incr;

		UK_ASSERT(lcpu_state_is_online(new));
	} while (ukarch_compare_exchange_sync((int *)&lcpu->state, old,
					      new) != new);

	return 1;
}

int ukplat_lcpu_run(const __lcpuidx lcpuidx[], unsigned int *num,
		    const struct ukplat_lcpu_func *fn, unsigned long flags)
{
	__lcpuid this_cpu_id = ukplat_lcpu_id();
	struct lcpu *lcpu;
	unsigned int n, i;
	int rc;

	UK_ASSERT(((lcpuidx) && (num)) || ((!lcpuidx) && (!num)));
	UK_ASSERT(fn);

	lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id)
			continue;

		/* Try to transition state to a higher busy level.
		 * We ignore CPUs that are not online
		 */
		if (!lcpu_transition_safe(lcpu, 1))
			continue;

		/* We successfully performed the state transition. Now queue
		 * the function and trigger its execution
		 */
		while (1) {
			rc = lcpu_arch_run(lcpu, fn, flags);
			if (unlikely(rc)) {
				/* Retry if we could not enqueue the function
				 * and it is ok to block
				 */
				if ((rc == -EAGAIN) &&
				    (!(flags & UKPLAT_LCPU_RFLG_DONOTBLOCK)))
					continue;

				/* Try to transition back one busy level. We
				 * don't care if the CPU is no longer online
				 */
				lcpu_transition_safe(lcpu, -1);

				return rc;
			}

			break;
		}
	}

	return 0;
}

int ukplat_lcpu_wait(const __lcpuidx lcpuidx[], unsigned int *num,
		     __nsec timeout)
{
	__lcpuid this_cpu_id = ukplat_lcpu_id();
	struct lcpu *lcpu;
	unsigned int n, i;
	int state;
	__nsec end;

	UK_ASSERT(((lcpuidx) && (num)) || ((!lcpuidx) && (!num)));

	if (timeout > 0)
		end = ukplat_monotonic_clock() + timeout;

	lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id)
			continue;

		/* Perform a busy wait until we reach IDLE state. However, we
		 * do not want to wait on HALTED or OFFLINE CPUs. So we are
		 * continuing while the LCPU is in INIT or BUSY state and the
		 * timeout has not been reached.
		 */
		while (1) {
			state = UK_READ_ONCE(lcpu->state);

			if ((state == LCPU_STATE_OFFLINE) ||
			    (state == LCPU_STATE_HALTED))
				break;

			if (state == LCPU_STATE_IDLE)
				break;

			if (timeout && (ukplat_monotonic_clock() >= end))
				return -ETIMEDOUT; /* Timed out */
		}
	}

	return 0;
}

int ukplat_lcpu_wakeup(const __lcpuidx lcpuidx[], unsigned int *num)
{
	__lcpuid this_cpu_id = ukplat_lcpu_id();
	struct lcpu *lcpu;
	unsigned int n, i;
	int rc;

	UK_ASSERT(((lcpuidx) && (num)) || ((!lcpuidx) && (!num)));

	lcpu_lcpuidx_list_foreach(lcpuidx, num, n, i, lcpu) {
		if (lcpu->id == this_cpu_id)
			continue;

		/* We ignore CPUs that are not online. Note that the CPU may
		 * change to HALTED state afterwards. However, that is not a
		 * problem, as the halt loop will return to sleep after the
		 * wakeup
		 */
		if (!lcpu_state_is_online(lcpu->state))
			continue;

		rc = lcpu_arch_wakeup(lcpu);
		if (unlikely(rc))
			return rc;
	}

	return 0;
}
#endif /* CONFIG_HAVE_SMP */
