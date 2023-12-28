/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/process.h>

#include "sigset.h"
#include "signal.h"
#include "siginfo.h"

static void uk_sigact_term(int __unused sig)
{
	pid_t tid = uk_gettid();

	uk_pr_warn("tid %d terminated by signal\n", tid);
	uk_posix_process_kill(tid2ukthread(uk_gettid()));
}

static void uk_sigact_ign(int __unused sig)
{
	UK_BUG(); /* We should never reach this point */
}

static void uk_sigact_core(int __unused sig)
{
	uk_pr_warn("%d: SIG_CORE not supported, falling back to SIG_TERM\n",
		   sig);
	uk_sigact_term(sig);
}

static void uk_sigact_stop(int __unused sig)
{
	uk_pr_warn("SIG_STOP not supported\n");
}

static void uk_sigact_cont(int __unused sig)
{
	uk_pr_warn("SIG_CONT not supported\n");
}

/* Checks whether a signal can be delivered to a given thread, depending
 * on the thread's mask, whether the process chooses to ingores this signal,
 * or whether the process uses a default disposition of ignore.
 *
 * Does NOT check permissions.
 */
static inline bool is_deliverable(struct posix_thread *pthread, int signum)
{
	struct posix_process *proc;

	UK_ASSERT(pthread);
	UK_ASSERT(signum);

	proc = tid2pprocess(pthread->tid);
	UK_ASSERT(proc);

	return (bool)(!IS_MASKED(pthread, signum) &&
		      !IS_IGNORED(proc, signum));
}

static void handle_self(struct uk_signal *sig, const struct k_sigaction *ks,
			struct uk_syscall_ctx *usc)
{
	struct posix_process *this_process __maybe_unused;
	ucontext_t ucontext __maybe_unused;
	stack_t *altstack __maybe_unused;
	struct ukarch_sysregs *sysregs;
	unsigned long saved_auxsp;
	__uptr ulsp;

	UK_ASSERT(sig);
	UK_ASSERT(ks);
	UK_ASSERT(usc);

	uk_pr_debug("tid %d handling signal %d, handler: 0x%lx, flags: 0x%lx\n",
		    uk_gettid(), sig->siginfo.si_signo, (__u64)ks->ks_handler,
		    ks->ks_flags);

	if (ks->ks_flags & SA_ONSTACK) {
		this_process = pid2pprocess(uk_getpid());
		altstack = &this_process->signal->altstack;

		/* Make sure sigaltstack() does not modify the altstack state
		 * while we are executing on it.
		 */
		uk_mutex_lock(&this_process->signal->lock);
		UK_ASSERT(altstack->ss_flags & SS_DISABLE);
		UK_ASSERT(!(altstack->ss_flags & SS_ONSTACK));
		altstack->ss_flags &= ~SS_DISABLE;
		altstack->ss_flags |= SS_ONSTACK;
		uk_mutex_unlock(&this_process->signal->lock);

		uk_pr_debug("Using altstack, ss_sp: 0x%lx, ss_size: 0x%lx\n",
			    (__u64)altstack->ss_sp, altstack->ss_size);

		UK_ASSERT(altstack->ss_sp);
		UK_ASSERT(altstack->ss_flags != SS_ONSTACK);

		ulsp = ukarch_gen_sp(altstack->ss_sp, altstack->ss_size);
	} else {
		ulsp = usc->regs.rsp;
		uk_pr_debug("Using the application stack @ 0x%lx\n", ulsp);
	}

	/* Signal handlers can make syscalls, that can invoke signal handlers,
	 * that can make syscalls, and so on. We need to support nesting.
	 * Since on syscall entry we switch to the auxp, nexted syscalls will
	 * corrupt the previous syscall's stack. To avoid that from happening,
	 * save the base auxsp of this syscall. Once the handler returns we will
	 * restore the original value. Moreover, prepare a new base auxsp for
	 * the nested syscall based on the current (aligned) value of the auxsp.
	 */
	saved_auxsp = uk_thread_current()->auxsp;

	UK_ASSERT(IS_ALIGNED(saved_auxsp, UKARCH_ECTX_ALIGN));
	UK_ASSERT(ukarch_read_sp() != saved_auxsp);

	/* Assuming that we are operating on the auxsp and that
	 * the sp has moved down since entry, align down to
	 * UKARCH_ECTX_ALIGN so that a nested syscall finds the
	 * auxsp aligned. Make sure we reserve enough space for
	 * pprocess_signal_arch_jmp_handler() to save our general
	 * purpose registers before we jump to the application
	 * handler.
	 */
	uk_thread_current()->auxsp = ALIGN_DOWN(ukarch_read_sp() -
						sizeof(struct __regs),
						UKARCH_ECTX_ALIGN);
	ukplat_lcpu_set_auxsp(uk_thread_current()->auxsp);

	/* Switch to the user's sysregs before calling the signal handler */
	sysregs = &usc->sysregs;
	ukarch_sysregs_switch_ul(sysregs);

	if (ks->ks_flags & SA_SIGINFO) {
		pprocess_signal_arch_set_ucontext(usc, &ucontext);
		pprocess_signal_arch_jmp_handler(sig->siginfo.si_signo,
						 &sig->siginfo, &ucontext,
						 ks->ks_handler, (void *)ulsp);
		pprocess_signal_arch_get_ucontext(&ucontext, usc);
	} else {
		pprocess_signal_arch_jmp_handler(sig->siginfo.si_signo,
						 NULL, NULL, ks->ks_handler,
						 (void *)ulsp);
	}

	/* Switch back to the uk sysregs */
	ukarch_sysregs_switch_uk(sysregs);

	/* Restore the auxsp */
	uk_thread_current()->auxsp = saved_auxsp;
	ukplat_lcpu_set_auxsp(uk_thread_current()->auxsp);

	if (ks->ks_flags & SA_ONSTACK) {
		uk_mutex_lock(&this_process->signal->lock);
		UK_ASSERT(altstack->ss_flags & SS_ONSTACK);
		UK_ASSERT(!(altstack->ss_flags & SS_DISABLE));
		altstack->ss_flags &= ~SS_ONSTACK;
		altstack->ss_flags |= SS_DISABLE;
		uk_mutex_unlock(&this_process->signal->lock);
	}
}

/* Deliver a signal to a thread. The caller must check that the signal
 * is not masked by the handling thread or ignored by the process or its
 * default disposition.
 */
static int do_deliver(struct posix_thread *pthread, struct uk_signal *sig,
		      struct uk_syscall_ctx *usc)
{
	struct posix_process *pproc;
	sigset_t saved_mask;
	int signum;

	UK_ASSERT(sig && sig->siginfo.si_signo);
	UK_ASSERT(pthread);

	pproc = tid2pprocess(pthread->tid);
	UK_ASSERT(pproc);

	signum = sig->siginfo.si_signo;

	/* Execute the default action if a signal handler is not defined */
	if (pproc->signal->sigaction[signum].ks_handler == SIG_DFL) {
		/* Standard signals: Invoke signal-specific default action */
		if (signum < SIGRTMIN) {
			if (UK_BIT(signum) & SIGACT_CORE_MASK)
				uk_sigact_core(signum);
			else if (UK_BIT(signum) & SIGACT_TERM_MASK)
				uk_sigact_term(signum);
			else if (UK_BIT(signum) & SIGACT_STOP_MASK)
				uk_sigact_stop(signum);
			else if (UK_BIT(signum) & SIGACT_CONT_MASK)
				uk_sigact_cont(signum);
			else if (UK_BIT(signum) & SIGACT_IGN_MASK)
				uk_sigact_ign(signum);
			else
				UK_BUG();
		} else {
			/* Real-time signals: SIG_TERM by default */
			uk_sigact_term(signum);
		}
		return 0;
	}

	uk_mutex_lock(&pproc->signal->lock);

	/* Save original mask and apply mask from sigaction */
	saved_mask = pthread->signal->mask;
	pthread->signal->mask |= pproc->signal->sigaction[signum].ks_mask;

	/* Also add this signal to masked signals */
	if (!(pproc->signal->sigaction[signum].ks_flags & SA_NODEFER))
		SET_MASKED(pthread, signum);

	uk_mutex_unlock(&pproc->signal->lock);

	/* Execute handler */
	handle_self(sig, &pproc->signal->sigaction[signum], usc);

	uk_mutex_lock(&pproc->signal->lock);

	/* Restore original mask */
	pthread->signal->mask = saved_mask;

	/* If SA_RESETHAND flag is set, restore the default handler */
	if (pproc->signal->sigaction[signum].ks_flags & SA_RESETHAND)
		pprocess_signal_sigaction_clear(&pproc->signal->sigaction[signum]);

	uk_mutex_unlock(&pproc->signal->lock);

	return 0;
}

/* Deliver pending signals of a given process. We deliver each
 * pengind signal to the first thread that doesn't mask that
 * signal.
 */
static int deliver_pending_proc(struct posix_process *proc,
				struct uk_syscall_ctx *usc)
{
	struct posix_thread *thread, *threadn;
	struct posix_thread *this_thread;
	struct uk_signal *sig;
	int handled_cnt = 0;
	bool handled;

	UK_ASSERT(proc);
	UK_ASSERT(usc);

	this_thread = tid2pthread(uk_gettid());

	for (int i = 0; i < SIG_ARRAY_COUNT; i++) {
		handled = false;

		/* Skip if the process ignores this signal altogether */
		if (IS_IGNORED(proc, i))
			continue;

		/* POSIX specifies that if a signal targets the
		 * current process / thread, then at least one
		 * signal for this process /thread must be
		 * delivered before the syscall returns, as long as:
		 *
		 * 1. No other thread has that signal unblocked
		 * 2. No other thread is in sigwait() for that signal (TODO)
		 */
		uk_process_foreach_pthread(proc, thread, threadn) {
			if (thread->tid == this_thread->tid)
				continue;

			if (IS_MASKED(thread, i))
				continue;

			while ((sig = pprocess_signal_dequeue_p(proc, i))) {
				do_deliver(thread, sig, usc);
				uk_signal_free(proc->_a, sig);
				handled = true;
				handled_cnt++;
			}
			break;
		}

		/* Try to deliver to this thread */
		if (!handled) {
			if (IS_MASKED(this_thread, i))
				continue;

			while ((sig = pprocess_signal_dequeue_p(proc, i))) {
				do_deliver(this_thread, sig, usc);
				uk_signal_free(proc->_a, sig);
				handled = true;
				handled_cnt++;
			}
		}
	}

	return handled_cnt;
}

static int deliver_pending_thread(struct posix_thread *thread,
				  struct uk_syscall_ctx *usc)
{
	struct posix_process *proc __maybe_unused;
	struct uk_signal *sig;
	int handled = 0;

	proc = tid2pprocess(thread->tid);
	UK_ASSERT(proc);

	/* Deliver this thread's signals. SUS requires that RT signals
	 * must be delivered starting from the lowest signal number.
	 * Delivery order of standard signals is undefined. We deliver
	 * all signals in order.
	 */
	for (int i = 1; i < SIG_ARRAY_COUNT; i++) {
		if (!is_deliverable(thread, i))
			continue;

		while ((sig = pprocess_signal_dequeue_t(thread, i))) {
			do_deliver(thread, sig, usc);
			uk_signal_free(thread->_a, sig);
			handled++;
		}
	}

	return handled;
}

void uk_signal_deliver(struct uk_syscall_ctx *usc)
{
	struct posix_thread *pthread;
	struct posix_process *pproc;
	pid_t tid = uk_gettid();

	UK_ASSERT(usc);

	pthread = tid2pthread(tid);
	UK_ASSERT(pthread);

	pproc = pid2pprocess(uk_getpid());
	UK_ASSERT(pproc);

	/* If there's SIGKILL pending, kill the process right away */
	if (IS_PENDING(pproc->signal->sigqueue, SIGKILL)) {
		uk_pr_info("SIGKILL tid %d\n", tid);
		uk_sigact_term(SIGKILL);
		return;
	}

	/* SIGSTOP / SIGCONT should have been already ignored by rt_sigaction */
	UK_ASSERT(!IS_PENDING(pproc->signal->sigqueue, SIGSTOP));
	UK_ASSERT(!IS_PENDING(pproc->signal->sigqueue, SIGCONT));

	/* Deliver all pending signals of this process & thread */
	deliver_pending_proc(pproc, usc);
	if (pthread->state == POSIX_THREAD_RUNNING ||
	    pthread->state == POSIX_THREAD_BLOCKED_SIGNAL)
		deliver_pending_thread(pthread, usc);
}
