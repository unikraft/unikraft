/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * Parts are copyright by other contributors. Please refer to copyright notices
 * in the individual source files, and to the git commit log, for a more accurate
 * list of copyright holders.
 *
 * OSv is open-source software, distributed under the 3-clause BSD license:
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Cloudius Systems, Ltd. nor the names of its
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* adapted from OSv */

#include <errno.h>

#include <uk/alloc.h>
#include <uk/sched.h>
#include <signal.h>
#include <uk/thread.h>
#include <uk/uk_signal.h>
#include <uk/essentials.h>
#include <unistd.h>

/*
 * Tries to deliver a pending signal to the current thread
 * Used only with a waiting thread
 *
 * Returns: 0 if no signal was delivered, 1 if a signal was delivered
 */
static int uk_get_awaited_signal(void)
{
	siginfo_t *siginfo;
	struct uk_signal *signal;
	struct uk_thread_sig *ptr;

	ptr = _UK_TH_SIG;

	/* try to deliver thread pending signal */
	signal = uk_sig_th_get_pending_any(ptr, ptr->wait.awaited);

	if (signal) {
		/* set awaited signal */
		ptr->wait.received_signal = signal->info;

		/* remove it from the list of pending signals */
		uk_list_del(&signal->list_node);
		uk_sigdelset(&ptr->pending, signal->info.si_signo);
		uk_free(uk_alloc_get_default(), signal);
		return 1;
	}

	/* try to deliver process pending signal */
	siginfo = uk_sig_proc_get_pending_any(ptr->wait.awaited);

	if (siginfo) {
		ptr->wait.received_signal = *siginfo;

		/* remove it from the list of pending signals */
		uk_remove_proc_signal(siginfo->si_signo);
		return 1;
	}

	return 0;
}

/* TODO: We do not support any sa_flags besides SA_SIGINFO */
int
sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	struct uk_list_head *i;
	struct uk_thread_sig *th_sig;
	struct uk_signal *signal;

	if (!uk_sig_is_valid(signum) ||
			signum == SIGKILL ||
			signum == SIGSTOP) {
		errno = EINVAL;
		return -1;
	}

	if (oldact)
		*oldact = uk_proc_sig.sigaction[signum - 1];

	if (act) {
		/* TODO: SA_NODEFER */
		uk_proc_sig.sigaction[signum - 1] = *act;
		uk_sigaddset(&uk_proc_sig.sigaction[signum - 1].sa_mask,
			     signum);

		/* remove signal from where it is pending */
		if (is_sig_ign(act)) {
			/* remove it from proc */
			uk_remove_proc_signal(signum);

			/* remove it from threads*/
			uk_list_for_each(i, &uk_proc_sig.thread_sig_list) {
				th_sig = uk_list_entry(i, struct uk_thread_sig,
						       list_node);

				signal = uk_sig_th_get_pending(th_sig, signum);

				if (signal) {
					/*
					 * remove it from the list of
					 * pending signalsi
					 */
					uk_list_del(&signal->list_node);
					uk_sigdelset(&th_sig->pending,
						     signal->info.si_signo);
					uk_free(uk_alloc_get_default(), signal);
				}
			}
		}
	}

	return 0;
}

static sighandler_t uk_signal(int signum, sighandler_t handler, int sa_flags)
{
	struct sigaction old;
	struct sigaction act = {
		.sa_handler = handler,
		.sa_flags = sa_flags
	};

	if (sigaction(signum, &act, &old) < 0)
		return SIG_ERR;

	if (old.sa_flags & SA_SIGINFO)
		return NULL;
	else
		return old.sa_handler;
}

sighandler_t signal(int signum, sighandler_t handler)
{
	/* SA_RESTART <- BSD signal semantics */
	return uk_signal(signum, handler, SA_RESTART);
}

int sigpending(sigset_t *set)
{
	struct uk_thread_sig *ptr;

	ptr = _UK_TH_SIG;

	uk_sigcopyset(set, &ptr->pending);
	uk_sigorset(set, &uk_proc_sig.pending);

	return 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	return uk_thread_sigmask(how, set, oldset);
}

int sigsuspend(const sigset_t *mask)
{
	/* If the signals are ignored, this doesn't return <- POSIX */

	sigset_t cleaned_mask, tmp;
	struct uk_thread_sig *ptr;

	uk_sigcopyset(&cleaned_mask, mask);
	uk_sigset_remove_unmaskable(&cleaned_mask);

	ptr = _UK_TH_SIG;

	uk_sigcopyset(&ptr->wait.awaited, &cleaned_mask);

	/* we are waiting for all the signals that aren't blocked */
	uk_sigreverseset(&ptr->wait.awaited);

	/* change mask */
	uk_sigcopyset(&tmp, &ptr->mask);
	uk_sigcopyset(&ptr->mask, &cleaned_mask);

	while (1) {
		/* try to deliver a pending signal */
		if (uk_get_awaited_signal())
			break;

		/* block, yield */
		uk_thread_block(uk_thread_current());
		ptr->wait.status = UK_SIG_WAITING;
		uk_sched_yield();
	}

	ptr->wait.status = UK_SIG_NOT_WAITING;

	/* execute handler */
	uk_execute_handler(ptr->wait.received_signal);

	/* restore mask
	 *
	 * We restore the mask here because we are technically done with
	 * sigsuspend and the mask must be restored at the end of sigsuspend
	 */
	uk_sigcopyset(&ptr->mask, &tmp);

	/* execute other pending signals */
	uk_sig_handle_signals();

	errno = EINTR;
	return -1; /* always returns -1 and sets errno to EINTR */
}

int sigwait(const sigset_t *set, int *sig)
{
	/*
	 * If the signals are ignored, this doesn't return <- TODO: POSIX ??
	 *
	 * POSIX states that the signals in set must have been blocked before
	 * calling sigwait, otherwise behavior is undefined -> for us the
	 * behavior is not caring -> even if the signal is not blocked sigwait
	 * will still accept it
	 *
	 * NOTE: this function is not signal safe
	 */

	int signals_executed;
	sigset_t cleaned_set, awaited_save;
	struct uk_thread_sig *ptr;

	uk_sigcopyset(&cleaned_set, set);
	uk_sigset_remove_unmaskable(&cleaned_set);

	if (uk_sigisempty(&cleaned_set))
		return EINVAL;

	ptr = _UK_TH_SIG;

	uk_sigcopyset(&ptr->wait.awaited, &cleaned_set);

	/* save awaited signals */
	awaited_save = ptr->wait.awaited;

	while (1) {
		if (uk_get_awaited_signal())
			break;

		/*
		 * sigwait allows signals to be received while
		 * waiting so handle them
		 */
		ptr->wait.status = UK_SIG_NOT_WAITING;
		signals_executed = uk_sig_handle_signals();

		if (signals_executed) {
			/*
			 * awaited might be changed by other waiting
			 * done while handling the signal
			 */
			ptr->wait.awaited = awaited_save;

			/*
			 * we might have raised / received a waiting signal
			 * while handling the others
			 */
			if (uk_get_awaited_signal())
				break;
		}

		/* block, yield */
		uk_thread_block(uk_thread_current());
		ptr->wait.status = UK_SIG_WAITING;
		uk_sched_yield();
	}

	ptr->wait.status = UK_SIG_NOT_WAITING;

	/* do not execute handler, set received signal */
	*sig = ptr->wait.received_signal.si_signo;

	/* execute other pending signals */
	uk_sig_handle_signals();

	return 0; /* returns positive errno */
}

/*
 * Search for a thread that does not have the signal blocked
 * If all of the threads have the signal blocked, add it to process
 * pending signals
 */
int kill(pid_t pid, int sig)
{
	/*
	 * POSIX.1 requires that if a process sends a signal to itself, and the
	 * sending thread does not have the signal blocked, and no other thread
	 * has it unblocked or is waiting for it in sigwait(3), at least one
	 * unblocked signal must be delivered to the sending thread before the
	 * kill() returns.
	 *
	 * FIXME: we don't implement this ^
	 */

	siginfo_t siginfo;
	struct uk_list_head *i;
	struct uk_thread_sig *th_sig;


	if (pid != 1 && pid != 0 && pid != -1) {
		errno = ESRCH;
		return -1;
	}

	if (!uk_sig_is_valid(sig)) {
		errno = EINVAL;
		return -1;
	}

	/* setup siginfo */
	uk_sig_init_siginfo(&siginfo, sig);

	uk_list_for_each(i, &uk_proc_sig.thread_sig_list) {
		th_sig = uk_list_entry(i, struct uk_thread_sig, list_node);

		if (uk_deliver_proc_signal(th_sig, &siginfo) > 0)
			return 0;
	}

	/* didn't find any thread that could accept this signal */
	uk_add_proc_signal(&siginfo);

	return 0;
}

int raise(int sig)
{
	return uk_sig_thread_kill(uk_thread_current(), sig);
}

/**
 * Stubbing the function support from requiring signal.
 * Stubs taken from newlib
 */
unsigned int alarm(unsigned int seconds __unused)
{
	return 0;
}

int siginterrupt(int sig __unused, int flag __unused)
{
	return 0;
}

int pause(void)
{
	return 0;
}
