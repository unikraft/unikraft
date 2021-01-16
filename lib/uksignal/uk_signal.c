/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors:  Mihai Pogonaru <pogonarumihai@gmail.com>
 *		     Teodora Serbanescu <teo.serbanescu16@gmail.com>
 *		     Felipe Huici <felipe.huici@neclab.eu>
 *		     Bernard Rizzo <b.rizzo@student.uliege.be>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest.
 * All rights reserved.
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
 */
#include <errno.h>

#include <uk/alloc.h>
#include <uk/print.h>
#include <signal.h>
#include <uk/thread.h>
#include <uk/uk_signal.h>

struct uk_proc_sig uk_proc_sig;

int uk_proc_sig_init(struct uk_proc_sig *sig)
{
	int i;

	sigemptyset(&sig->pending);

	for (i = 0; i < NSIG - 1; ++i) {
		/* TODO: set ign to the ones that should be ign */
		sig->sigaction[i].sa_handler = SIG_DFL;
		sigemptyset(&sig->sigaction[i].sa_mask);
		sig->sigaction[i].sa_flags = 0;
	}

	UK_INIT_LIST_HEAD(&sig->thread_sig_list);
	return 0;
}

int uk_thread_sig_init(struct uk_thread_sig *sig)
{
	sigemptyset(&sig->mask);

	sig->wait.status = UK_SIG_NOT_WAITING;
	sigemptyset(&sig->wait.awaited);

	sigemptyset(&sig->pending);
	UK_INIT_LIST_HEAD(&sig->pending_signals);

	uk_list_add(&sig->list_node, &uk_proc_sig.thread_sig_list);
	return 0;
}

void uk_thread_sig_uninit(struct uk_thread_sig *sig)
{
	/* Clear pending signals */
	struct uk_list_head *i, *tmp;
	struct uk_signal *signal;

	uk_list_del(&sig->list_node);

	uk_list_for_each_safe(i, tmp, &sig->pending_signals) {
		signal = uk_list_entry(i, struct uk_signal, list_node);

		uk_list_del(&signal->list_node);
		uk_free(uk_alloc_get_default(), signal);
	}
}

int uk_sig_handle_signals(void)
{
	/*
	 * Do not run pending signals if the thread is waiting
	 *
	 * Iterate over pending signals
	 * Check if the signal is blocked
	 *
	 * Before running the handler, remove it from pending
	 */

	int handled = 0;
	sigset_t executable, rmask;
	struct uk_signal *signal;
	struct uk_thread_sig *ptr;

	ptr = _UK_TH_SIG;

	/*
	 * handle_sigals will be called from
	 * the respective waiting function
	 */
	if (ptr->wait.status != UK_SIG_NOT_WAITING)
		return 0;

	/* reverse mask */
	uk_sigcopyset(&rmask, &ptr->mask);
	uk_sigreverseset(&rmask);

	/* calculate executable signals */
	uk_sigcopyset(&executable, &rmask);
	uk_sigandset(&executable, &ptr->pending);

	while (!uk_sigisempty(&executable)) {
		signal = uk_list_first_entry(&ptr->pending_signals,
					     struct uk_signal, list_node);

		/* move it last if it's blocked */
		if (uk_sigismember(&ptr->mask, signal->info.si_signo)) {
			uk_list_del(&signal->list_node);
			uk_list_add_tail(&signal->list_node,
					 &ptr->pending_signals);
			continue;
		}

		/* remove from the list */
		uk_list_del(&signal->list_node);

		/* clear pending status */
		uk_sigdelset(&ptr->pending, signal->info.si_signo);

		/* execute */
		uk_execute_handler(signal->info);

		uk_free(uk_alloc_get_default(), signal);
		handled++;

		/* calculate executable signals */
		uk_sigcopyset(&executable, &rmask);
		uk_sigandset(&executable, &ptr->pending);
	}

	return handled;
}

/*
 * returns:
 *  >0 - on success
 *  0  - if signal is ignored / already pending
 *  <0 - on failure
 */
static int
uk_deliver_signal_unmasked(struct uk_thread_sig *th_sig, siginfo_t *sig)
{
	struct uk_thread *tid;
	struct uk_signal *uk_sig;

	/* If the signal is ignored, we don't deliver it */
	if (is_sig_ign(&uk_proc_sig.sigaction[sig->si_signo - 1]))
		return 0;

	/* If it is already pending, we don't deliver it */
	if (uk_sigismember(&th_sig->pending, sig->si_signo))
		return 0;

	uk_sig = uk_malloc(uk_alloc_get_default(), sizeof(*uk_sig));
	if (!uk_sig) {
		uk_pr_warn("Could not allocate uk_signal");
		return -ENOMEM;
	}

	uk_sig->info = *sig;
	uk_list_add(&uk_sig->list_node, &th_sig->pending_signals);

	uk_sigaddset(&th_sig->pending, sig->si_signo);

	/* check if we need to wake the thread */
	if (th_sig->wait.status != UK_SIG_NOT_WAITING &&
			th_sig->wait.status != UK_SIG_WAITING_SCHED) {
		th_sig->wait.status = UK_SIG_WAITING_SCHED;

		tid = __containerof(th_sig, struct uk_thread,
				    signals_container);
		uk_thread_wake(tid);
	}

	return 1;
}

int uk_sig_thread_kill(struct uk_thread *tid, int sig)
{
	siginfo_t siginfo;
	struct uk_signal *signal;
	struct uk_thread_sig *ptr;

	if (!uk_sig_is_valid(sig)) {
		errno = EINVAL;
		return -1;
	}

	ptr = &tid->signals_container;

	/* setup siginfo */
	uk_sig_init_siginfo(&siginfo, sig);

	/* check if we are sending this to ourself */
	if (&tid->signals_container == _UK_TH_SIG) {
		/* if it's not masked just run it */
		if (!uk_sigismember(&ptr->mask, sig)) {
			/* remove the signal from pending */
			signal = uk_sig_th_get_pending(ptr, sig);

			if (signal) {
				uk_list_del(&signal->list_node);
				uk_sigdelset(&ptr->pending, sig);
				uk_free(uk_alloc_get_default(), signal);
			}

			uk_execute_handler(siginfo);
			return 0;
		}
	}

	uk_deliver_signal_unmasked(ptr, &siginfo);

	return 0;
}

/*
 * Used to deliver pending signals after a mask change
 *
 * Since mask doesn't affect the deliverance of signals
 * directed to threads (this is handled by uk_sig_handle_signals),
 * we are trying to deliver only pending process signals
 */
static inline void uk_deliver_signals_maskchange(void)
{
	int i, ret;
	sigset_t to_send;
	struct uk_thread_sig *ptr;

	ptr = _UK_TH_SIG;

	uk_sigcopyset(&to_send, &ptr->mask);
	uk_sigandset(&to_send, &uk_proc_sig.pending);

	for (i = 1; i < NSIG; ++i) {
		if (uk_sigisempty(&to_send))
			break;

		if (uk_sigismember(&to_send, i)) {
			ret = uk_deliver_proc_signal(ptr,
					&uk_proc_sig.pending_signals[i - 1]);

			if (ret > 0) {
				uk_remove_proc_signal(i);
				uk_sigdelset(&to_send, i);
			}
		}
	}
}

/*
 * TODO: if we add rt sig, don't allow the
 * two real-time sig that are used internally by the NPTL
 * threading implementation. Also for all the functions - ignore
 * those signals silently
 */
int uk_thread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
	/*
	 * running this inside a handler has no effect outside the handler
	 * since the mask is restored in execute_handler
	 */

	sigset_t *mask, tmp;
	struct uk_thread_sig *ptr;

	ptr = _UK_TH_SIG;
	mask = &(ptr->mask);

	if (oldset)
		*oldset = *mask;

	if (set) {
		switch (how) {
		case SIG_BLOCK:
			uk_sigorset(mask, set);
			break;
		case SIG_UNBLOCK:
			uk_sigcopyset(&tmp, set);
			uk_sigreverseset(&tmp);
			uk_sigandset(mask, &tmp);
			break;
		case SIG_SETMASK:
			uk_sigcopyset(mask, set);
			break;
		default:
			errno = EINVAL;
			return -1;
		}

		uk_sigset_remove_unmaskable(mask);

		/* Changed the mask, see if we can deliver
		 * any pending signals
		 */
		uk_deliver_signals_maskchange();

		uk_sig_handle_signals();
	}

	return 0;
}

struct uk_thread_sig *uk_crr_thread_sig_container(void)
{
	return &(uk_thread_current()->signals_container);
}

void uk_sig_init_siginfo(siginfo_t *siginfo, int sig)
{
	siginfo->si_signo = sig;

	/* TODO: add codes; get pid from getpid() */
	siginfo->si_code = 0;
	siginfo->si_pid = 1;
}

/* returns the uk_signal for sig if it is pending on thread */
struct uk_signal *uk_sig_th_get_pending(struct uk_thread_sig *th_sig, int sig)
{
	struct uk_list_head *i;
	struct uk_signal *signal;

	if (!uk_sigismember(&th_sig->pending, sig))
		return NULL;

	uk_list_for_each(i, &th_sig->pending_signals) {
		signal = uk_list_entry(i, struct uk_signal, list_node);

		if (signal->info.si_signo == sig)
			return signal;
	}

	/* NOT REACHED */
	return NULL;
}

/* returns the siginfo for sig if it is pending on proc */
siginfo_t *uk_sig_proc_get_pending(int sig)
{
	if (!uk_sigismember(&uk_proc_sig.pending, sig))
		return NULL;

	return &uk_proc_sig.pending_signals[sig - 1];
}

/*
 * returns the uk_signal for a signal from the given
 * set if it is pending on thread
 */
struct uk_signal *
uk_sig_th_get_pending_any(struct uk_thread_sig *th_sig, sigset_t set)
{
	sigset_t common;
	struct uk_list_head *i;
	struct uk_signal *signal;

	uk_sigcopyset(&common, &th_sig->pending);
	uk_sigandset(&common, &set);

	if (uk_sigisempty(&common))
		return NULL;

	uk_list_for_each(i, &th_sig->pending_signals) {
		signal = uk_list_entry(i, struct uk_signal, list_node);

		if (uk_sigismember(&common, signal->info.si_signo))
			return signal;
	}

	/* NOT REACHED */
	return NULL;
}

/*
 * returns the siginfo for a signal from the given
 * set if it is pending on proc
 */
siginfo_t *uk_sig_proc_get_pending_any(sigset_t set)
{
	int sig;
	sigset_t common;

	uk_sigcopyset(&common, &uk_proc_sig.pending);
	uk_sigandset(&common, &set);

	if (uk_sigisempty(&common))
		return NULL;

	for (sig = 1; sig < NSIG; ++sig)
		if (uk_sigismember(&common, sig))
			return &uk_proc_sig.pending_signals[sig - 1];

	/* NOT REACHED */
	return NULL;
}

/*
 * returns:
 *  >0 - on success
 *  0  - if signal is blocked / ignored / already pending
 *  <0 - on failure
 */
int
uk_deliver_proc_signal(struct uk_thread_sig *th_sig, siginfo_t *sig)
{
	if (uk_sigismember(&th_sig->mask, sig->si_signo))
		return 0;

	return uk_deliver_signal_unmasked(th_sig, sig);
}

void uk_execute_handler(siginfo_t sig)
{
	/*
	 * We save siginfo locally since it might change
	 * while the handler is runnnig
	 * For example, if we execute a waiting function inside
	 * the handler and we were already inside a waiting function
	 */

	sigset_t tmp;
	struct sigaction *act;
	struct uk_thread_sig *ptr;

	act = &uk_proc_sig.sigaction[sig.si_signo - 1];

	/*
	 * Check if the signal is ignored
	 *
	 * This should never happen since
	 * we never deliver ignored signals
	 */
	if (is_sig_ign(act)) {
		uk_pr_debug("Ignored signal %d\n",
				sig.si_signo);
		ukplat_crash();
	}

	/* our default handler is shutdown */
	if (is_sig_dfl(act)) {
		uk_pr_debug("Uncaught signal %d. Powering off.\n",
				sig.si_signo);
		ukplat_crash();
	}

	ptr = _UK_TH_SIG;

	/* change the mask */
	uk_sigcopyset(&tmp, &ptr->mask);
	uk_sigorset(&ptr->mask, &act->sa_mask);

	/* run the handler */
	if (act->sa_flags & SA_SIGINFO)
		act->sa_sigaction(sig.si_signo, &sig, NULL);
	else
		act->sa_handler(sig.si_signo);

	/* check if we need to reset handler */
	if (act->sa_flags & SA_RESETHAND) {
		act->sa_flags = 0;
		act->sa_handler = SIG_DFL;
	}

	/* restore the mask */
	uk_sigcopyset(&ptr->mask, &tmp);
}
