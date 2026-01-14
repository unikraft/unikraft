/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch/ctx.h>
#include <uk/essentials.h>
#include <uk/plat/config.h>
#include <uk/prio.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "process.h"
#include "sigset.h"
#include "signal.h"
#include "siginfo.h"

struct pprocess_signal_handler_ctx {
	void *handler_fn;
	void *handler_sp;
	siginfo_t *siginfo;
	ucontext_t *ucontext;
	int signo;
} __packed;

UK_CTASSERT(__offsetof(struct pprocess_signal_handler_ctx, handler_fn) ==
	    PPROCESS_SIGHNDL_CTX_FN_OFFS);
UK_CTASSERT(__offsetof(struct pprocess_signal_handler_ctx, handler_sp) ==
	    PPROCESS_SIGHNDL_CTX_SP_OFFS);
UK_CTASSERT(__offsetof(struct pprocess_signal_handler_ctx, signo) ==
	    PPROCESS_SIGHNDL_CTX_SN_OFFS);
UK_CTASSERT(__offsetof(struct pprocess_signal_handler_ctx, siginfo) ==
	    PPROCESS_SIGHNDL_CTX_SI_OFFS);
UK_CTASSERT(__offsetof(struct pprocess_signal_handler_ctx, ucontext) ==
	    PPROCESS_SIGHNDL_CTX_UC_OFFS);

/* Notice: We implement this in pure asm, as there is a number of issues
 *         with correctly preserving context in inline asm.
 */
void pprocess_signal_jmp_handler(struct pprocess_signal_handler_ctx *h,
				 struct ukarch_execenv *execenv);

static void uk_sigact_term(int __unused sig)
{
	struct posix_process *pprocess;

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	uk_pr_info("pid: %d terminated by signal\n", pprocess->pid);

	pprocess_exit(pprocess, POSIX_PROCESS_KILLED, sig);
	uk_sched_thread_exit(); /* noreturn */
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

static void handle_self(struct uk_signal *sig, const struct kern_sigaction *ks,
			struct ukarch_execenv *execenv)
{
	struct pprocess_signal_handler_ctx handler_ctx;
	ucontext_t ucontext;
	struct posix_process *this_process;
	stack_t *altstack;
	__uptr ulsp;

	UK_ASSERT(sig);
	UK_ASSERT(ks);
	UK_ASSERT(execenv);

	/* We expect to be operating on the aux stack */
	UK_ASSERT(SP_IN_AUXSP(uk_arch_read_sp(), uk_lcpu_get_auxsp()));

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
		ulsp = ALIGN_DOWN(uk_lcpu_regs_get(execenv->regs, SP),
				  UKARCH_SP_ALIGN);
		uk_pr_debug("Using the application stack @ 0x%lx\n", ulsp);
	}

	handler_ctx.handler_fn = ks->ks_handler;
	handler_ctx.handler_sp = (void *)ulsp;
	handler_ctx.signo = sig->siginfo.si_signo;

	if (ks->ks_flags & SA_SIGINFO) {
		handler_ctx.siginfo = &sig->siginfo;
		handler_ctx.ucontext = &ucontext;

		pprocess_signal_arch_set_ucontext(execenv, &ucontext);
		pprocess_signal_jmp_handler(&handler_ctx, execenv);
		pprocess_signal_arch_get_ucontext(&ucontext, execenv);
	} else {
		handler_ctx.siginfo = __NULL;
		handler_ctx.ucontext = __NULL;

		pprocess_signal_jmp_handler(&handler_ctx, execenv);
	}

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
static void do_deliver(struct posix_thread *pthread, struct uk_signal *sig,
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
		return;
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

			while ((sig = pprocess_signal_dequeue(proc, __NULL,
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

			while ((sig = pprocess_signal_dequeue(proc, __NULL,
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

/* We land here from the trap handler that executes in exception context.
 * Once we return, the trampoline will pass control back to the application.
 */
void sys_error_handler(struct ukarch_execenv *ee __unused, long arg)
{
	struct ukarch_auxspcb *auxspcb;
	struct sys_error_desc *error;
	struct posix_thread *pthread;
	struct posix_process *pproc;
	struct uk_signal sig = {0};

	/* Now we can enable IRQs */
	uk_lcpu_enable_irq();

	/* Get arg */
	error = (struct sys_error_desc *)arg;
	UK_ASSERT(error);

	/* Switch to uk sysregs */
	auxspcb = ukarch_auxsp_get_cb(error->auxsp);
	uk_lcpu_sysctx_load((struct uk_lcpu_sysctx *)auxspcb->uksysctx);

	/* Derive current process and thread */
	pproc = uk_pprocess_current();
	UK_ASSERT(pproc);

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	/* If there's a SIGKILL pending, kill the process right away */
	if (IS_PENDING(pproc->signal->sigqueue, SIGKILL)) {
		uk_sigact_term(SIGKILL);
		UK_BUG(); /* noreturn */
	}

	/* If the application masks or ignores this signal, panic.
	 * A general-purpose OS would terminate the application.
	 * Being a unikernel, we treat this as a non-recoverable
	 * error instead.
	 */
	if (!pprocess_signal_is_deliverable(pthread, error->signum))
		goto err_panic;

	if (KERN_SIGACTION(pproc, error->signum)->ks_handler == SIG_DFL)
		goto err_panic;

	/* Prepare siginfo */
	set_siginfo_kill(error->signum, &sig.siginfo);

	/* Execute standard delivery path */
	do_deliver(pthread, &sig, ee);

	return;

err_panic:
	/* FIXME: Cascading faulting */
	UK_CRASH("Cannot deliver SIGSEGV for pf at 0x%lx\n", error->vaddr);
}
