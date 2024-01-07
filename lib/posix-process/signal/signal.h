/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#ifndef __UK_SIGNAL_H__
#define __UK_SIGNAL_H__

#include <limits.h>
#include <signal.h>

#include <uk/alloc.h>
#include <uk/list.h>
#include <uk/process.h>
#include <uk/semaphore.h>

#include "process.h"
#include "sigset.h"

#define SIG_ARRAY_COUNT		_NSIG /* SIGRTMAX + 1 */

/* Check if signal number is valid */
#define IS_VALID(_signum)						\
	((_signum) > 0 && (_signum) <= SIGRTMAX)

/* Check whether signal number is pending in sigqueue */
#define IS_PENDING(_sigqueue, _signum)					\
	uk_sigismember(&(_sigqueue).pending, (_signum))

/* Set signal number in sigqueue */
#define SET_PENDING(_sigqueue, _signum)					\
	uk_sigaddset(&(_sigqueue).pending, (_signum))

/* Clear a signal from a sigqueue */
#define RESET_PENDING(_sigqueue, _signum)				\
	uk_sigdelset(&(_sigqueue).pending, (_signum))

/* Check if posix thread masks signal number */
#define IS_MASKED(_t, _signum)						\
	uk_sigismember(&(_t)->signal->mask, (_signum))

/* Sets a posix thread's mask of the given signal number */
#define SET_MASKED(_t, _signum)						\
	uk_sigaddset(&(_t)->signal->mask, (_signum))

/* Check if signal is ignored by a process. A signal is ignored if it
 * is explicitly set to IGNORE by sigaction, or if it is set to its
 * default action and that action is IGNORE.
 */
#define IS_IGNORED(_pproc, _signum)					  \
	(((_pproc)->signal->sigaction[(_signum)].ks_handler == SIG_IGN) ||\
	 ((_pproc)->signal->sigaction[(_signum)].ks_handler == SIG_DFL && \
	  UK_BIT(_signum) & SIGACT_IGN_MASK))

/* Allocate and return `struct uk_signal` or NULL */
#define uk_signal_alloc(_alloc)						\
	uk_zalloc(_alloc, sizeof(struct uk_signal))

/* Free a previously allocated `struct uk_signal` */
#define uk_signal_free(_alloc, _sig)					\
	uk_free(_alloc, _sig)

/* Kernel sigaction passed to rt_sigaction(). The order of the
 * elements of this struct is different of the `struct sigaction`
 * passed to the sigaction() glibc wrapper.
 */
struct uk_sigaction {
	void (*ks_handler)(int signo);
	unsigned long ks_flags;
	void (*ks_restorer)(void);
	uk_sigset_t ks_mask;
};

/* Descriptor of a pending signal.
 *
 * Each pending signal is described as a set of
 * information `siginfo`. This includes the signal
 * number, sending pid, address of faulting
 * instruction, and more. The individual fields of
 * `siginfo_t` are described in `man siginfo_t`.
 */
struct uk_signal {
	siginfo_t siginfo;
	struct uk_list_head list_head;
};

/* Descriptor of a signal queue.
 *
 * Pending signals are expressed in a signal set, that
 * provides information on which signal numbers are pending.
 *
 * By POSIX, standard signals are queued once regardless of
 * the number of times the signal was issued by the sender.
 * Real-time signals allow in-order delivery of multiple
 * instances of a given signal.
 *
 * This structure maintains an list of `struct uk_signal` for
 * each signal number. Lists of standard signals can only
 * contain a single item.
 */
struct uk_signal_queue {
	uk_sigset_t pending;
	struct uk_list_head list_head[SIG_ARRAY_COUNT];
};

/* Signal descriptor of a posix_process.
 *
 * A process may queue up to `queued_max` signals,
 * the value of which is set at init time.
 * POSIX requires at least _POSIX_SIGQUEUE_MAX
 * signals, while Linux allows the user to configure
 * that value via rlimit.
 *
 * Signals can be targeting the process, or individual
 * threads. Signals pending for the process are queued
 * in `sigqueue`. Signals pending for individual
 * threads are queued in thread-specific queues (see
 * `struct uk_signal_tdesc`).
 *
 * Each signal defines a signal action, which may
 * be the default one or a custom one specified by
 * calling `rt_sigaction`. Signal actions are defined
 * at the process level, i.e. are common for all
 * threads of a given process.
 */
struct uk_signal_pdesc {
	__sz queued_count;
	__sz queued_max;
	struct uk_signal_queue sigqueue;
	struct uk_sigaction *sigaction;
	struct uk_mutex lock;
};

/* Signal descriptor of a posix_thread.
 *
 * Each thread maintains its own signal mask.
 * That mask applies to signals targeting
 * that thread, and also impacts the delivery
 * of process-wide signals, as these are
 * delivered to the first thread that does
 * not mask that signal.
 *
 * Each thread also maintains its own signal
 * queue, as besides signals targeting the
 * process, signals can target individual
 * threads. For details on signal queues see
 * the description of `struct uk_signal_queue`.
 */
struct uk_signal_tdesc {
	uk_sigset_t mask;
	uk_sigset_t sigwait_set;
	struct uk_signal_queue sigqueue;
	struct uk_semaphore deliver_semaphore;
	struct uk_semaphore pending_semaphore;
};

/* Allocate signal descriptor of a posix process */
int pprocess_signal_pdesc_alloc(struct posix_process *proc);

/* Allocate signal descriptor of a posix thread */
int pprocess_signal_tdesc_alloc(struct posix_thread *thread);

/* Initialize signal descriptors of a posix process */
int pprocess_signal_pdesc_init(struct posix_process *proc);

/* Initialize signal descriptors of a posix thread */
int pprocess_signal_tdesc_init(struct posix_thread *thread);

/* Free signal descritors of a posix process
 *
 * This function is safe to call at thread termination as
 * we reject signals once the process exits.
 */
void pprocess_signal_pdesc_free(struct posix_process *process);

/* Free signal descritors of a posix thread
 *
 * This function is safe to call at thread termination as
 * we reject signals once the process exits.
 */
void pprocess_signal_tdesc_free(struct posix_thread *thread);

/* Dequeue a signal sent to process */
struct uk_signal *pprocess_signal_dequeue_p(struct posix_process *pproc,
					    int signum);

/* Dequeue a signal sent to thread */
struct uk_signal *pprocess_signal_dequeue_t(struct posix_thread *pthread,
					    int signum);

/* Clears the fields of an instance of struct uk_sigaction */
void pprocess_signal_sigaction_clear(struct uk_sigaction *sa);

/* Clear pending signal(s) from process. If the signum corresponds to
 * an std-signal, it clears that an signal from pending. Otherwise,
 * if signum corresponding an rt-signal it clears all instances from
 * the queue.
 */
void pprocess_signal_clear_pending(struct posix_process *proc, int signum);

/* Get next pending signal of a given thread */
struct uk_signal *pprocess_signal_next_pending_t(struct posix_thread *pthread);

#endif /* __UK_SIGNAL_H__ */
