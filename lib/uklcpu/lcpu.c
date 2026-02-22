/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2022, Karlsruhe Institute of Technology (KIT)
 *                     All rights reserved.
 * Copyright (c) 2022, University Politehnica of Bucharest.
 *                     All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/assert.h>
#include <uk/atomic.h>
#if CONFIG_HAVE_SMP
#include <uk/intctlr.h>
#endif /* CONFIG_HAVE_SMP */
#include <uk/lcpu.h>
#include <uk/plat/config.h>
#include <uk/plat/time.h>
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
__uk_pcpuvar struct uk_lcpu uk_lcpus;

static const struct uk_lcpu_pm_ops *pm_ops;

int uk_lcpu_pm_ops_register(const struct uk_lcpu_pm_ops *ops)
{
	if (unlikely(!ops))
		return -EINVAL;

	pm_ops = ops;
	return 0;
}

struct uk_lcpu *uk_lcpu_get_current(void)
{
	return uk_pcpuvar_current_ptr_get(uk_lcpus);
}

__isr __u64 uk_lcpu_get_current_idx_in_except(void)
{
	return (uk_arch_read_sp() - uk_pal_except_get_except_stack_base()) /
		(CPU_EXCEPT_STACK_SIZE * 3);
}

int uk_lcpu_init(struct uk_lcpu *this_lcpu)
{
	/*
	 * NOTE: Do not use anything that might need initialized exception
	 * traps until after lcpu_arch_init(), as traps might not be
	 * initialized for this CPU yet!
	 */

	/* Initialize the bootstrap CPU */
	if (uk_lcpu_is_bsp(this_lcpu)) {
		this_lcpu->state = UK_LCPU_STATE_INIT;
	} else {
		/* We should already be in INIT state for secondary CPUs */
		UK_ASSERT(this_lcpu->state == UK_LCPU_STATE_INIT);
	}

	UK_ASSERT(uk_lcpu_irqs_disabled());

#if CONFIG_HAVE_SMP
	this_lcpu->fn.fn = __NULL;
#endif /* CONFIG_HAVE_SMP */

	/* Write back changes before marking CPU as online */
	uk_arch_wmb();

	/* Put the CPU in busy state. This will mark it as online. After this
	 * point, functions may be queued to the CPU. However, IRQs are still
	 * disabled.
	 */
	this_lcpu->state = UK_LCPU_STATE_BUSY0;

	return uk_pal_except_init();
}

static void __noreturn lcpu_halt(struct uk_lcpu *this_cpu, int error_code)
{
	uk_lcpu_disable_irq();

	this_cpu->state = UK_LCPU_STATE_HALTED;
	this_cpu->error_code = error_code;

	while (1) {
		/* Although we should not be able to recover via regular
		 * interrupts, we might receive NMIs so loop to be safe.
		 */
		if (!pm_ops || !pm_ops->halt)
			uk_arch_spinwait();
		else
			pm_ops->halt();
	}

	__builtin_unreachable();
}

void __noreturn uk_lcpu_halt(void)
{
	lcpu_halt(uk_lcpu_get_current(), 0);
}

void uk_lcpu_halt_irq(void)
{
	UK_ASSERT(uk_lcpu_irqs_disabled());

	if (unlikely(!pm_ops || !pm_ops->halt_irq)) {
		uk_pal_enable_irq();
		uk_arch_spinwait();
	} else {
		pm_ops->halt_irq();
	}
}

void uk_lcpu_halt_irq_until(__nsec until)
{
	UK_ASSERT(uk_lcpu_irqs_disabled());

	time_block_until(until);
}

#if CONFIG_HAVE_SMP
int uk_lcpu_fn_enqueue(struct uk_lcpu *lcpu, const struct uk_lcpu_func *fn)
{
	void (*old_fn)(struct uk_lcpu_regs *, void *);

	UK_ASSERT(fn->fn);

	old_fn = uk_load_n(&lcpu->fn.fn);

	/* Check if the slot is empty */
	if (old_fn != __NULL)
		return -EAGAIN;

	/* It is empty, try to store the function */
	if (uk_compare_exchange_sync(&lcpu->fn.fn, old_fn,
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
	uk_arch_wmb();

	return 0;
}

static void lcpu_fn_dequeue(struct uk_lcpu *this_lcpu, struct uk_lcpu_func *fn)
{
	*fn = this_lcpu->fn;

	/* Ensure that we have captured the whole function object */
	uk_arch_rmb();

	UK_ASSERT(fn->fn);

	/* Free the slot. Another function object can be queued afterwards */
	this_lcpu->fn.fn = __NULL;
}

static int lcpu_ipi_run_handler(void *args __unused)
{
	struct uk_lcpu *this_lcpu = uk_lcpu_get_current();
	struct uk_lcpu_func fn;

	lcpu_fn_dequeue(this_lcpu, &fn);

	/* TODO: Provide the register snapshot from the trap frame */
	fn.fn(__NULL, fn.user);

	/* If we had a transition from BUSY to HALTED in fn, we would
	 * not reach this code but sit in the error halt loop. We can
	 * thus safely just decrement without worrying about the HALTED
	 * state.
	 */
	UK_ASSERT(uk_lcpu_state_is_busy(this_lcpu->state));
	uk_dec(&this_lcpu->state);

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

const unsigned long * const uk_lcpu_run_irqv = &_lcpu_run_irqv;
const unsigned long * const uk_lcpu_wakeup_irqv = &_lcpu_wakeup_irqv;

int uk_lcpu_mp_init(unsigned long run_irq, unsigned long wakeup_irq)
{
	int rc;

	/* Make sure this is run on the BSP only */
	UK_ASSERT(uk_lcpu_current_is_bsp());

	/* Register the lcpu_run and lcpu_wakeup interrupt handlers */
	rc = uk_intctlr_irq_register(run_irq, lcpu_ipi_run_handler, __NULL);
	if (unlikely(rc)) {
		uk_pr_crit("Could not register handler for IPI IRQ %ld\n",
			   run_irq);
		return rc;
	}

	rc = uk_intctlr_irq_register(wakeup_irq, lcpu_ipi_wakeup_handler,
				     __NULL);
	if (unlikely(rc)) {
		uk_pr_crit("Could not register handler for wakeup IRQ %ld\n",
			   wakeup_irq);
		return rc;
	}

	_lcpu_run_irqv = run_irq;
	_lcpu_wakeup_irqv = wakeup_irq;

	return 0;
}

void __weak __noreturn uk_lcpu_entry_default(struct uk_lcpu *this_lcpu)
{
	__uptr sentry, sstackp, sarg;
	int rc;

	UK_ASSERT(!uk_lcpu_is_bsp(this_lcpu));

	/* Finish initialization. As there is nothing to return to, we
	 * just enter halted state if an error occurs.
	 */
	rc = uk_lcpu_init(this_lcpu);
	if (unlikely(rc))
		lcpu_halt(this_lcpu, rc);

	/* If the user supplied an entry function jump to it */
	sentry = uk_pcpuvar_current_get(UK_LCPU_SENTRY_SYM);
	sstackp = uk_pcpuvar_current_get(UK_LCPU_SSTACKP_SYM);
	sarg = uk_pcpuvar_current_get(UK_LCPU_SARG_SYM);
	if (sentry && sentry != (__uptr)uk_lcpu_entry_default) {
		/* Does not return */
		uk_arch_jump_to_with_arg(sstackp, sentry, sarg);
	} else {
		/* We are coming from BUSY0 state and want to transition to
		 * IDLE state. However, there can be functions queued already
		 * so we have to use a decrement here.
		 */
		uk_dec(&this_lcpu->state);

		/* Enable IRQs. If there are functions queued we will
		 * immediately jump to the IRQ handler.
		 */
		uk_lcpu_enable_irq();
		while (1) {
			/* Besides interrupts in general, the halt can be
			 * interrupted by calls to uk_lcpu_run().
			 */
			uk_lcpu_halt();
		}
	}
}

int uk_lcpu_start(const __u64 lcpuidx[], unsigned int *num,
		  __uptr sp[], __uptr entry[],
		  unsigned long flags __unused)
{
	__u64 this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
	const int new = UK_LCPU_STATE_INIT;
	struct uk_lcpu *lcpu;
	int old, rc = 0, rc2;
	unsigned int i, n;

	UK_ASSERT(lcpuidx);
	UK_ASSERT(num);
	UK_ASSERT(sp);

	if (unlikely(!pm_ops || !pm_ops->start))
		return -ENODEV;

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
	if (unlikely(!pm_ops->post_start))
		return -ENODEV;
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */

	for (i = 0, n = *num; i < n; i++) {
		if (uk_pcpuvar_lval(lcpuidx[i],
				    uk_pcpuvar_cpu_id) == this_cpu_id)
			continue;

		lcpu = &uk_pcpuvar_lval(lcpuidx[i], uk_lcpus);
retry:
		old = uk_load_n(&lcpu->state);

		/* We ignore CPUs that are already started */
		if (unlikely(old != UK_LCPU_STATE_OFFLINE)) {
			uk_pr_warn("Failed to start CPU 0x%lx: not offline\n",
				   uk_pcpuvar_lval(lcpuidx[i],
						   uk_pcpuvar_cpu_id));
			continue;
		}

		/* Try to acquire the CPU for initialization. If another thread
		 * was faster, we will return to the state comparison and
		 * report that the CPU is not offline.
		 */
		if (uk_compare_exchange_sync((int *)&lcpu->state, old,
					     new) != new)
			goto retry;

		UK_ASSERT(lcpu->state == UK_LCPU_STATE_INIT);

		/* Setup startup arguments.
		 * Since we are ignoring the executing CPU, we must keep a
		 * separate counter to index the arguments.
		 */
		uk_pcpuvar_lval(lcpuidx[i], UK_LCPU_SENTRY_SYM) =
			(entry && entry[i]) ?
				entry[i] :
				(__uptr)uk_lcpu_entry_default;
		uk_pcpuvar_lval(lcpuidx[i], UK_LCPU_SSTACKP_SYM) = sp[i];
		uk_pcpuvar_lval(lcpuidx[i], UK_LCPU_SARG_SYM) = (__uptr)lcpu;

		/* Ensure that the startup arguments have been written back
		 * before issuing the startup call
		 */
		uk_arch_wmb();

		rc = pm_ops->start(lcpuidx[i]);
		if (unlikely(rc)) {
			lcpu->state = UK_LCPU_STATE_HALTED;
			lcpu->error_code = rc;

			/* There is a serious problem. Stop here. The caller
			 * can skip the CPU by using the value of *num.
			 */
			break;
		}
	}

	*num = i;

#if CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP
	/* At this point, i has been set to the number of successfully
	 * started CPUs. So if there has been an error, we won't touch
	 * any CPUs not started.
	 */
	rc2 = pm_ops->post_start(lcpuidx, &i);
	if (unlikely(rc2)) {
		UK_ASSERT(i <= *num);
		*num = i;

		/* Return the first error */
		return (rc) ? rc : rc2;
	}
#endif /* CONFIG_HAVE_CPU_MULTI_PHASE_STARTUP */

	UK_ASSERT(num == __NULL || *num == i);
	return rc;
}

static inline int lcpu_transition_safe(struct uk_lcpu *lcpu, int incr)
{
	int old, new;

	/* Transition the CPU to a different busy level. The CPU could not be
	 * online or fall into a halted state at any moment, we thus cannot
	 * just atomically in-/decrement the state. Otherwise, we might corrupt
	 * the non-online state.
	 */
	do {
		old = uk_load_n(&lcpu->state);

		/* We must not change the state if the CPU is not online */
		if (!uk_lcpu_state_is_online(old))
			return 0;

		UK_ASSERT(old <= INT_MAX - incr);
		UK_ASSERT(old >= INT_MIN + incr);
		new = old + incr;

		UK_ASSERT(uk_lcpu_state_is_online(new));
	} while (uk_compare_exchange_sync((int *)&lcpu->state, old,
					  new) != new);

	return 1;
}

int uk_lcpu_run(const __u64 lcpuidx[], unsigned int *num,
		const struct uk_lcpu_func *fn, unsigned long flags)
{
	__u64 this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
	struct uk_lcpu *lcpu;
	unsigned int n, i;
	int rc = 0;

	UK_ASSERT(lcpuidx);
	UK_ASSERT(num);
	UK_ASSERT(fn);

	for (i = 0, n = *num; i < n; i++) {
		if (uk_pcpuvar_lval(lcpuidx[i],
				    uk_pcpuvar_cpu_id) == this_cpu_id)
			continue;

		lcpu = &uk_pcpuvar_lval(lcpuidx[i], uk_lcpus);

		/* Try to transition state to a higher busy level.
		 * We ignore CPUs that are not online
		 */
		if (!lcpu_transition_safe(lcpu, 1))
			continue;

		/* We successfully performed the state transition. Now queue
		 * the function and trigger its execution
		 */
		while (1) {
			rc = uk_lcpu_fn_enqueue(lcpu, fn);
			if (unlikely(rc))
				goto lcpu_run_err;

			rc = uk_pal_except_send_ipi(uk_pcpuvar_lval(lcpuidx[i],
							uk_pcpuvar_cpu_id),
						    *uk_lcpu_run_irqv);
			if (unlikely(rc)) {
				/* Retry if we could not enqueue the function
				 * and it is ok to block
				 */
				if ((rc == -EAGAIN) &&
				    (!(flags & UK_LCPU_RFLG_DONOTBLOCK)))
					continue;

				/* Try to transition back one busy level. We
				 * don't care if the CPU is no longer online
				 */
				lcpu_transition_safe(lcpu, -1);
				goto lcpu_run_err;
			}

			break;
		}
	}

lcpu_run_err:
	*num = i;
	return rc;
}

int uk_lcpu_wait(const __u64 lcpuidx[], unsigned int *num,
		 __nsec timeout)
{
	__u64 this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
	struct uk_lcpu *lcpu;
	unsigned int n, i;
	int state, rc = 0;
	__nsec end;

	UK_ASSERT(lcpuidx);
	UK_ASSERT(num);

	if (timeout > 0)
		end = ukplat_monotonic_clock() + timeout;

	for (i = 0, n = *num; i < n; i++) {
		if (uk_pcpuvar_lval(lcpuidx[i],
				    uk_pcpuvar_cpu_id) == this_cpu_id)
			continue;

		lcpu = &uk_pcpuvar_lval(lcpuidx[i], uk_lcpus);

		/* Perform a busy wait until we reach IDLE state. However, we
		 * do not want to wait on HALTED or OFFLINE CPUs. So we are
		 * continuing while the LCPU is in INIT or BUSY state and the
		 * timeout has not been reached.
		 */
		while (1) {
			state = UK_READ_ONCE(lcpu->state);

			if ((state == UK_LCPU_STATE_OFFLINE) ||
			    (state == UK_LCPU_STATE_HALTED))
				break;

			if (state == UK_LCPU_STATE_IDLE)
				break;

			if (timeout && (ukplat_monotonic_clock() >= end)) {
				rc = -ETIMEDOUT; /* Timed out */
				goto lcpu_wait_err;
			}
		}
	}

lcpu_wait_err:
	*num = i;
	return rc;
}

int uk_lcpu_wakeup(const __u64 lcpuidx[], unsigned int *num)
{
	__u64 this_cpu_id = uk_pcpuvar_current_get(uk_pcpuvar_cpu_id);
	struct uk_lcpu *lcpu;
	unsigned int n, i;
	int rc = 0;

	UK_ASSERT(lcpuidx);
	UK_ASSERT(num);

	for (i = 0, n = *num; i < n; i++) {
		if (uk_pcpuvar_lval(lcpuidx[i],
				    uk_pcpuvar_cpu_id) == this_cpu_id)
			continue;

		lcpu = &uk_pcpuvar_lval(lcpuidx[i], uk_lcpus);

		/* We ignore CPUs that are not online. Note that the CPU may
		 * change to HALTED state afterwards. However, that is not a
		 * problem, as the halt loop will return to sleep after the
		 * wakeup
		 */
		if (!uk_lcpu_state_is_online(lcpu->state))
			continue;

		rc = uk_pal_except_send_ipi(uk_pcpuvar_lval(lcpuidx[i],
							    uk_pcpuvar_cpu_id),
				     *uk_lcpu_wakeup_irqv);
		if (unlikely(rc))
			goto lcpu_wakeup_err;
	}

lcpu_wakeup_err:
	*num = i;
	return rc;
}
#endif /* CONFIG_HAVE_SMP */
