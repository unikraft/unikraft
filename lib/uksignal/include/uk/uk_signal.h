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
#ifndef __UK_UK_SIGNAL_H__
#define __UK_UK_SIGNAL_H__

#include <uk/list.h>
#include <uk/bits/sigset.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _UK_TH_SIG uk_crr_thread_sig_container()

struct uk_thread;

struct uk_signal {
	siginfo_t info;
	struct uk_list_head list_node;
};

/* TODO: add synchronization */

struct uk_proc_sig {
	/* used as a bitmap for pending signals */
	sigset_t pending;
	/* pending signals - valid only if
	 * corresponding bit in pending is set
	 */
	siginfo_t pending_signals[NSIG - 1];
	/* signal handlers for this process */
	struct sigaction sigaction[NSIG - 1];
	/* list of uk_thread_sig from the threads of the proc */
	struct uk_list_head thread_sig_list;
};

extern struct uk_proc_sig uk_proc_sig;

enum uk_sig_waiting {
	UK_SIG_NOT_WAITING = 0,
	UK_SIG_WAITING = 1,
	UK_SIG_WAITING_SCHED = 2
};

struct uk_thread_sig_wait {
	/*
	 * waiting status
	 *
	 * values:
	 *	UK_SIG_NOT_WAITING - thread is not waiting
	 *	UK_SIG_WAITING - thread is waiting for a signal
	 *	UK_SIG_WAITING_SCHED - thread is waiting to be scheduled
	 */
	enum uk_sig_waiting status;
	/* used as a bitmap for awaited signals */
	sigset_t awaited;
	/* awaited signal received */
	siginfo_t received_signal;
};

struct uk_thread_sig {
	/* blocked signals */
	sigset_t mask;
	/* used as a bitmap for pending signals */
	sigset_t pending;
	/* list of pending signals */
	struct uk_list_head pending_signals;
	/* signal waiting state */
	struct uk_thread_sig_wait wait;
	/* node for the thread_sig_list from the proc */
	struct uk_list_head list_node;
};

/* returns number of executed signal handlers */
int uk_sig_handle_signals(void);

int uk_proc_sig_init(struct uk_proc_sig *sig);
int uk_thread_sig_init(struct uk_thread_sig *sig);
void uk_thread_sig_uninit(struct uk_thread_sig *sig);

/* TODO: replace sched thread_kill? */
int uk_sig_thread_kill(struct uk_thread *tid, int sig);
int uk_thread_sigmask(int how, const sigset_t *set, sigset_t *oldset);

/* internal use */
static inline int uk_sig_is_valid(int sig)
{
	return (sig < NSIG && sig > 0);
}

static inline void uk_sigset_remove_unmaskable(sigset_t *sig)
{
	uk_sigdelset(sig, SIGKILL);
	uk_sigdelset(sig, SIGSTOP);
}

static inline void uk_add_proc_signal(siginfo_t *sig)
{
	uk_proc_sig.pending_signals[sig->si_signo - 1] = *sig;
	uk_sigaddset(&uk_proc_sig.pending, sig->si_signo);
}

static inline void uk_remove_proc_signal(int sig)
{
	uk_sigdelset(&uk_proc_sig.pending, sig);
}

/* maybe move to sched */
struct uk_thread_sig *uk_crr_thread_sig_container(void);
void uk_sig_init_siginfo(siginfo_t *siginfo, int sig);

/* returns the uk_signal for sig if it is pending on thread */
struct uk_signal *
uk_sig_th_get_pending(struct uk_thread_sig *th_sig, int sig);

/* returns the siginfo for sig if it is pending on proc */
siginfo_t *uk_sig_proc_get_pending(int sig);

/*
 * returns the uk_signal for a signal from the given
 * set if it is pending on thread
 */
struct uk_signal *
uk_sig_th_get_pending_any(struct uk_thread_sig *th_sig, sigset_t set);

/*
 * returns the siginfo for a signal from the given
 * set if it is pending on proc
 */
siginfo_t *uk_sig_proc_get_pending_any(sigset_t set);

int
uk_deliver_proc_signal(struct uk_thread_sig *th_sig, siginfo_t *sig);
void uk_execute_handler(siginfo_t sig);

#ifdef __cplusplus
}
#endif

#endif /* __UK_UK_SIGNAL_H__ */
