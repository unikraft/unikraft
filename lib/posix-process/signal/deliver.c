/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/prio.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "sigset.h"
#include "signal.h"
#include "siginfo.h"

/* asm */
void pprocess_signal_call_handler_with_stack(int signum, siginfo_t *si,
					     ucontext_t *ctx, void *handler,
					     void *sp);

static void uk_sigact_term(int __unused sig)
{
	uk_pr_warn("tid %d terminated by signal\n", uk_sys_gettid());
	uk_posix_process_kill(uk_thread_current());
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

bool pprocess_signal_is_deliverable(struct posix_thread *pthread, int signum)
{
	struct posix_process *proc;

	UK_ASSERT(pthread);
	UK_ASSERT(signum);

	proc = tid2pprocess(pthread->tid);
	UK_ASSERT(proc);

	return (!IS_MASKED(pthread, signum) && !IS_IGNORED(proc, signum));
}

/* Switches to application context and jumps to handler.
 * Restores uk context on return.
 */
static
void jmp_handler(struct ukarch_execenv *execenv, int signum, siginfo_t *si,
		 ucontext_t *ctx, void *handler, void *sp)
{
	struct ukarch_sysctx uk_sysctx;

	ukarch_sysctx_store(&uk_sysctx);
	ukarch_sysctx_load(&execenv->sysctx);

	/* Notice: We implement this trampoline in asm as there is a ton of
	 *         challenges involved with correctly preserving context
	 *         with inline asm.
	 */
	pprocess_signal_call_handler_with_stack(signum, si, ctx, handler, sp);

	ukarch_sysctx_load(&uk_sysctx);
}

static void handle_self(struct uk_signal *sig, const struct kern_sigaction *ks,
			struct ukarch_execenv *execenv)
{
	ucontext_t ucontext __maybe_unused;
	struct posix_process *this_process;
	struct ukarch_auxspcb *auxspcb;
	__uptr fp, saved_fp;
	stack_t *altstack;
	__uptr ulsp;

	UK_ASSERT(sig);
	UK_ASSERT(ks);
	UK_ASSERT(execenv);

	this_process = uk_pprocess_current();
	UK_ASSERT(this_process);

	altstack = &this_process->signal->altstack;

	uk_pr_debug("tid %d handling signal %d, handler: 0x%lx, flags: 0x%lx\n",
		    uk_sys_gettid(), sig->siginfo.si_signo,
		    (__u64)ks->ks_handler,
		    ks->ks_flags);

	/* TODO: Make sure sigaltstack() does not modify the altstack state
	 * while we are executing on it, neither sigaction() modifies
	 * sa_flags->SA_ONSTACK.
	 */

	if ((ks->ks_flags & SA_ONSTACK) && !(altstack->ss_flags & SS_DISABLE)) {
		UK_ASSERT(altstack->ss_sp);
		UK_ASSERT(!(altstack->ss_flags & SS_ONSTACK));

		altstack->ss_flags |= SS_ONSTACK;

		uk_pr_debug("Using altstack, ss_sp: 0x%lx, ss_size: 0x%lx\n",
			    (__u64)altstack->ss_sp, altstack->ss_size);

		ulsp = ukarch_gen_sp(altstack->ss_sp, altstack->ss_size);
	} else {
		ulsp = ALIGN_DOWN(ukarch_regs_get_sp(&execenv->regs),
				  UKARCH_SP_ALIGN);
		uk_pr_debug("Using the application stack @ 0x%lx\n", ulsp);
	}

	/* Signal handlers can make syscalls, that can invoke signal handlers,
	 * that can make syscalls, and so on. We need to support nesting.
	 * Since on syscall entry we switch to the auxsp, nested syscalls will
	 * corrupt the previous syscall's stack. To avoid that from happening,
	 * save the base auxsp of this syscall. Once the handler returns we will
	 * restore the original value. Moreover, prepare a new base auxsp for
	 * the nested syscall based on the current (aligned) value of the auxsp.
	 */

	/* Assuming that we are operating on the auxsp and that
	 * the sp has moved down since entry, align down to
	 * UKARCH_AUXSP_ALIGN so that a nested syscall finds the
	 * auxsp aligned. Make sure we reserve enough space for
	 * pprocess_signal_arch_jmp_handler() to save our general
	 * purpose registers before we jump to the application
	 * handler.
	 */
	auxspcb = ukarch_auxsp_get_cb(uk_thread_current()->auxsp);
	saved_fp = ukarch_auxspcb_get_curr_fp(auxspcb);

	fp = ALIGN_DOWN(ukarch_read_sp(), UKARCH_AUXSP_ALIGN);
	ukarch_auxspcb_set_curr_fp(auxspcb, fp);

	if (ks->ks_flags & SA_SIGINFO) {
		pprocess_signal_arch_set_ucontext(execenv, &ucontext);
		jmp_handler(execenv, sig->siginfo.si_signo,
			    &sig->siginfo, &ucontext,
			    ks->ks_handler, (void *)ulsp);
		pprocess_signal_arch_get_ucontext(&ucontext, execenv);
	} else {
		jmp_handler(execenv, sig->siginfo.si_signo,
			    NULL, NULL, ks->ks_handler,
			    (void *)ulsp);
	}

	/* Restore the aux stack's fp */
	ukarch_auxspcb_set_curr_fp(auxspcb, saved_fp);

	if (ks->ks_flags & SA_ONSTACK) {
		UK_ASSERT(altstack->ss_flags & SS_ONSTACK);
		UK_ASSERT(!(altstack->ss_flags & SS_DISABLE));
		altstack->ss_flags &= ~SS_ONSTACK;
	}
}

/* Deliver a signal to a thread. The caller must check that the signal
 * is not masked by the handling thread or ignored by the process or its
 * default disposition.
 */
static int do_deliver(struct posix_thread *pthread, struct uk_signal *sig,
		      struct ukarch_execenv *execenv)
{
	struct posix_process *pproc;
	uk_sigset_t saved_mask;
	int signum;

	UK_ASSERT(sig && sig->siginfo.si_signo);
	UK_ASSERT(pthread);

	pproc = tid2pprocess(pthread->tid);
	UK_ASSERT(pproc);

	signum = sig->siginfo.si_signo;

	/* Execute the default action if a signal handler is not defined */
	if (KERN_SIGACTION(pproc, signum)->ks_handler == SIG_DFL) {
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

	/* Save original mask and apply mask from sigaction */
	saved_mask = pthread->signal->mask;
	uk_sigorset(&pthread->signal->mask,
		    &KERN_SIGACTION(pproc, signum)->ks_mask);

	/* Also add this signal to masked signals */
	if (!(KERN_SIGACTION(pproc, signum)->ks_flags & SA_NODEFER))
		SET_MASKED(pthread, signum);

	/* Execute handler */
	handle_self(sig, KERN_SIGACTION(pproc, signum), execenv);

	/* Restore original mask */
	pthread->signal->mask = saved_mask;

	/* If SA_RESETHAND flag is set, restore the default handler */
	if (KERN_SIGACTION(pproc, signum)->ks_flags & SA_RESETHAND)
		pprocess_signal_sigaction_clear(KERN_SIGACTION(pproc, signum));

	return 0;
}

/* Deliver pending signals of a given process. We deliver each
 * pending signal to the first thread that doesn't mask that
 * signal.
 */
static int deliver_pending_proc(struct posix_process *proc,
				struct ukarch_execenv *execenv)
{
	struct posix_thread *thread, *threadn;
	struct posix_thread *this_thread;
	struct uk_signal *sig;
	int handled_cnt = 0;
	bool handled;
	int signum;

	UK_ASSERT(proc);
	UK_ASSERT(execenv);

	this_thread = uk_pthread_current();
	UK_ASSERT(this_thread);

	pprocess_signal_foreach(signum) {
		handled = false;

		/* Skip if the process ignores this signal altogether */
		if (IS_IGNORED(proc, signum))
			continue;

		/* POSIX specifies that if a signal targets the
		 * current process / thread, then at least one
		 * signal for this process /thread must be
		 * delivered before the syscall returns, as long as:
		 *
		 * 1. No other thread has that signal unblocked
		 * 2. No other thread is in sigwait() for that signal (TODO)
		 */
		uk_pprocess_foreach_pthread(proc, thread, threadn) {
			if (thread->tid == this_thread->tid)
				continue;

			if (IS_MASKED(thread, signum))
				continue;

			while ((sig = pprocess_signal_dequeue(proc, NULL,
							      signum))) {
				do_deliver(thread, sig, execenv);
				uk_signal_free(proc->_a, sig);
				handled = true;
				handled_cnt++;
			}
			break;
		}

		/* Try to deliver to this thread */
		if (!handled) {
			if (IS_MASKED(this_thread, signum))
				continue;

			while ((sig = pprocess_signal_dequeue(proc, NULL,
							      signum))) {
				do_deliver(this_thread, sig, execenv);
				uk_signal_free(proc->_a, sig);
				handled = true;
				handled_cnt++;
			}
		}
	}

	return handled_cnt;
}

static int deliver_pending_thread(struct posix_thread *thread,
				  struct ukarch_execenv *execenv)
{
	struct posix_process *proc;
	struct uk_signal *sig;
	int handled = 0;
	int signum;

	proc = tid2pprocess(thread->tid);
	UK_ASSERT(proc);

	/* Deliver this thread's signals. SUS requires that RT signals
	 * must be delivered starting from the lowest signal number.
	 * Delivery order of standard signals is undefined. We deliver
	 * all signals in order.
	 */
	pprocess_signal_foreach(signum) {
		if (!pprocess_signal_is_deliverable(thread, signum))
			continue;

		while ((sig = pprocess_signal_dequeue(proc, thread, signum))) {
			do_deliver(thread, sig, execenv);
			uk_signal_free(thread->_a, sig);
			handled++;
		}
	}

	return handled;
}

static void uk_signal_deliver(struct uk_syscall_exit_ctx *exit_ctx)
{
	struct ukarch_execenv *execenv;
	struct posix_thread *pthread;
	struct posix_process *pproc;

	UK_ASSERT(exit_ctx);

	execenv = exit_ctx->execenv;

	/* FIXME: This can go away once we eliminate the last syscall made
	 *        by kernel context and pass execenv to syscalls in native
	 *        mode.
	 */
	if (!execenv)
		return;

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	pproc = uk_pprocess_current();
	UK_ASSERT(pproc);

	/* If there's SIGKILL pending, kill the process right away */
	if (IS_PENDING(pproc->signal->sigqueue, SIGKILL)) {
		uk_pr_info("SIGKILL tid %d\n", uk_sys_gettid());
		uk_sigact_term(SIGKILL);
		/* We assume that we always call this function
		 * on the running process.
		 */
		UK_BUG();
	}

	/* SIGSTOP / SIGCONT should have been already ignored by rt_sigaction */
	UK_ASSERT(!IS_PENDING(pproc->signal->sigqueue, SIGSTOP));
	UK_ASSERT(!IS_PENDING(pproc->signal->sigqueue, SIGCONT));

	/* Deliver all pending signals of this process & thread */
	deliver_pending_proc(pproc, execenv);
	if (pthread->state == POSIX_THREAD_RUNNING ||
	    pthread->state == POSIX_THREAD_BLOCKED_SIGNAL)
		deliver_pending_thread(pthread, execenv);
}

uk_syscall_exittab_prio(uk_signal_deliver, UK_PRIO_BEFORE(UK_PRIO_LATEST));
