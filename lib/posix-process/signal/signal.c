/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/process.h>
#include <uk/refcount.h>
#include <uk/semaphore.h>

#include "process.h"
#include "sigset.h"
#include "siginfo.h"
#include "signal.h"

/* Check permissions to signal target process.
 *
 * For a process to be permitted to send a signal to another
 * process, its real / effective uid must match the target's
 * real / saved set-user-id. We don't support these, so we
 * deliver to every process.
 */
#define signal_check_perm(_target_process) true

/* Enqueue signal for a thread or a process
 *
 * Setting pthread to NULL signifies that the signal
 * targets the process.
 */
static int pprocess_signal_enqueue(struct posix_process *pproc,
				   struct posix_thread *pthread,
				   struct uk_signal *sig)
{
	int signum;

	UK_ASSERT(sig);
	UK_ASSERT(pproc);

	if (pthread)
		UK_ASSERT(pthread->process == pproc);

	if (unlikely(pproc->signal->queued_count >= pproc->signal->queued_max))
		return -EAGAIN;

	signum = sig->siginfo.si_signo;

	if (pthread) {
		/* Standard signals can be queued once */
		if (signum < SIGRTMIN && IS_PENDING(pthread->signal->sigqueue, signum))
			return 0;

		uk_pr_debug("Queueing signal %d for tid %d (pid %d)\n",
			    signum, pthread->tid, pproc->pid);

		uk_list_add_tail(&sig->list_head,
				 &pthread->signal->sigqueue.list_head[signum]);
		SET_PENDING(pproc->signal->sigqueue, signum);
	} else {
		/* Standard signals can be queued once */
		if (signum < SIGRTMIN && IS_PENDING(pproc->signal->sigqueue, signum))
			return 0;

		uk_pr_debug("Queueing signal %d for pid %d\n",
			    signum, pproc->pid);

		uk_list_add_tail(&sig->list_head,
				 &pproc->signal->sigqueue.list_head[signum]);
		SET_PENDING(pproc->signal->sigqueue, signum);
	}

	pproc->signal->queued_count++;

	return 0;
}

struct uk_signal *pprocess_signal_dequeue(struct posix_process *pproc,
					  struct posix_thread *pthread,
					  int signum)
{
	struct uk_signal_queue *sigqueue;
	struct uk_signal *sig;

	UK_ASSERT(pproc);

	if (pthread) {
		UK_ASSERT(pthread->process == pproc);
		sigqueue = &pthread->signal->sigqueue;
	} else {
		sigqueue = &pproc->signal->sigqueue;
	}

	sig = uk_list_first_entry_or_null(&sigqueue->list_head[signum],
					  struct uk_signal,
					  list_head);
	if (!sig)
		return sig;

	uk_list_del(&sig->list_head);

	/* Reset pending bit if the signal queue is empty */
	if (uk_list_empty(&sigqueue->list_head[signum])) {
		if (pthread)
			RESET_PENDING(pthread->signal->sigqueue, signum);
		else
			RESET_PENDING(pproc->signal->sigqueue, signum);
	}

	pproc->signal->queued_count--;

	return sig;
}

struct uk_signal *pprocess_signal_next_pending_t(struct posix_thread *pthread)
{
	struct posix_process *pproc;
	struct uk_signal *sig;
	int signum;

	pproc = tid2pprocess(pthread->tid);
	UK_ASSERT(pproc);

	pprocess_signal_foreach(signum) {
		if (!pprocess_signal_is_deliverable(pthread, signum))
			continue;

		sig = pprocess_signal_dequeue(pproc, pthread, signum);
		if (sig)
			return sig;
	}

	return NULL;
}

void pprocess_signal_clear_pending(struct posix_process *proc, int signum)
{
	struct posix_thread *thread, *threadn;
	struct uk_signal *sig;

	if (IS_PENDING(proc->signal->sigqueue, signum)) {
		while ((sig = pprocess_signal_dequeue(proc, NULL, signum)))
			uk_signal_free(proc->_a, sig);
	}

	uk_pprocess_foreach_pthread(proc, thread, threadn) {
		if (!IS_PENDING(thread->signal->sigqueue, signum))
			continue;
		while ((sig = pprocess_signal_dequeue(proc, thread, signum)))
			uk_signal_free(thread->_a, sig);
	}
}

void pprocess_signal_sigaction_clear(struct kern_sigaction *ks)
{
	ks->ks_handler = SIG_DFL;
	ks->ks_flags = 0;
	ks->ks_restorer = NULL;
	uk_sigemptyset(&ks->ks_mask);
}

int pprocess_signal_sigaction_new(struct uk_alloc *alloc, struct uk_signal_pdesc *pd)
{
	pd->sigaction = uk_malloc(alloc, sizeof(struct uk_sigaction));
	if (unlikely(!pd->sigaction)) {
		uk_pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	pd->sigaction->alloc = alloc;
	uk_refcount_init(&pd->sigaction->refcnt, 1);

	return 0;
}

int pprocess_signal_send(struct posix_process *proc, int signum,
			 siginfo_t *siginfo)
{
	struct uk_signal *sig;
	int rc;

	sig = uk_signal_alloc(proc->_a);
	if (unlikely(!sig)) {
		uk_pr_err("Could not allocate signal\n");
		return -EAGAIN;
	}

	if (siginfo)
		set_siginfo_sigqueue(signum, &sig->siginfo, siginfo);
	else
		set_siginfo_kill(signum, &sig->siginfo);

	rc = pprocess_signal_enqueue(proc, NULL, sig);
	if (unlikely(rc)) {
		/* issue a warning as this may be temporary */
		uk_pr_warn("Could not queue signal\n");
		uk_signal_free(proc->_a, sig);
		return -EAGAIN;
	}

	return 0;
}

int pprocess_signal_process_do(pid_t pid, int signum, siginfo_t *siginfo)
{
	struct posix_process *pproc = NULL;
	struct posix_thread *thread, *threadn;
	int rc;

	if (unlikely(signum != 0 && !IS_VALID(signum)))
		return -EINVAL;

	if (unlikely(signum == SIGSTOP || signum == SIGCONT)) {
		uk_pr_warn("Process stop / resume not supported. Ignoring\n");
		return 0;
	}

	/* pid == 0 -> Send the signal to every process in the process group
	 *             of the calling process.
	 * pid < -1 -> Send the signal to every process in the process group
	 *             with ID equal to -pid.
	 */
	if (!pid || pid < -1) {
		uk_pr_warn("pgroups not supported, delivering to every process\n");
		pid = -1;
	}

	/* pid == -1 -> Send the signal to every process for which the
	 *              calling process has permissions to signal.
	 */
	if (pid == -1) {
		uk_pprocess_foreach(pproc) {
			if (!signal_check_perm(pproc))
				continue;

			if (IS_IGNORED(pproc, signum))
				continue;

			rc = pprocess_signal_send(pproc, signum, siginfo);
			if (unlikely(rc))
				return rc;
		}
		return 0;
	}

	/* Default: Send to process with pid */
	pproc = pid2pprocess(pid);
	if (unlikely(!pproc))
		return -ESRCH;

	if (unlikely(signal_check_perm(pproc) == false))
		return -EPERM;

	/* If signum == 0 do only pid exist & permissions check */
	if (!signum)
		return 0;

	/* If this signal is currently ignored, don't even try */
	if (IS_IGNORED(pproc, signum))
		return 0;

	rc = pprocess_signal_send(pproc, signum, siginfo);
	if (unlikely(rc))
		return rc;

	/* Wake up any threads that may be paused */
	uk_pprocess_foreach_pthread(pproc, thread, threadn) {
		if (thread->state == POSIX_THREAD_BLOCKED_SIGNAL)
			uk_semaphore_up(&thread->signal->deliver_semaphore);
	}

	return 0;
}

int pprocess_signal_thread_do(int tid, int signum, siginfo_t *siginfo)
{
	struct posix_thread *pthread;
	struct posix_process *pproc;
	struct uk_signal *sig;
	int rc;

	pthread = tid2pthread(tid);
	if (unlikely(!pthread))
		return -EINVAL;

	pproc = tid2pprocess(tid);
	UK_ASSERT(pproc);

	sig = uk_signal_alloc(pthread->_a);
	if (unlikely(!sig)) {
		uk_pr_err("Could not allocate signal\n");
		return -EAGAIN;
	}

	if (siginfo)
		set_siginfo_sigqueue(signum, &sig->siginfo, siginfo);
	else
		set_siginfo_kill(signum, &sig->siginfo);

	rc = pprocess_signal_enqueue(pproc, pthread, sig);
	if (unlikely(rc)) {
		/* issue a warning as this may be temporary */
		uk_pr_warn("Could not queue signal\n");
		uk_signal_free(pthread->_a, sig);
		return rc;
	}

	if (pthread->state == POSIX_THREAD_BLOCKED_SIGNAL)
		uk_semaphore_up(&pthread->signal->deliver_semaphore);

	return 0;
}

int pprocess_signal_pdesc_alloc(struct posix_process *process)
{
	UK_ASSERT(process);

	process->signal = uk_malloc(process->_a,
				    sizeof(struct uk_signal_pdesc));
	if (unlikely(!process->signal)) {
		uk_pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	return 0;
}

int pprocess_signal_tdesc_alloc(struct posix_thread *thread)
{
	UK_ASSERT(thread);

	thread->signal = uk_malloc(thread->_a, sizeof(struct uk_signal_tdesc));
	if (unlikely(!thread->signal)) {
		uk_pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	return 0;
}

int pprocess_signal_pdesc_init(struct posix_process *process)
{
	struct uk_signal_pdesc *pd;
	int signum;
	int rc;

	UK_ASSERT(process);
	UK_ASSERT(process->signal);

	pd = process->signal;

	pd->queued_count = 0;
	pd->queued_max = _POSIX_SIGQUEUE_MAX;

	uk_sigemptyset(&pd->sigqueue.pending);

	pprocess_signal_foreach(signum)
		UK_INIT_LIST_HEAD(&pd->sigqueue.list_head[signum]);

	rc = pprocess_signal_sigaction_new(process->_a, pd);
	if (unlikely(rc))
		return rc;

	pprocess_signal_foreach(signum)
		pprocess_signal_sigaction_clear(KERN_SIGACTION(process, signum));

	pd->altstack.ss_flags = SS_DISABLE;

	return 0;
}

int pprocess_signal_tdesc_init(struct posix_thread *thread)
{
	struct uk_signal_tdesc *td;
	int signum;

	UK_ASSERT(thread);
	UK_ASSERT(thread->signal);

	td = thread->signal;

	uk_sigemptyset(&td->mask);
	uk_sigemptyset(&td->sigqueue.pending);

	pprocess_signal_foreach(signum)
		UK_INIT_LIST_HEAD(&td->sigqueue.list_head[signum]);

	uk_semaphore_init(&thread->signal->pending_semaphore, 0);
	uk_semaphore_init(&thread->signal->deliver_semaphore, 0);

	return 0;
}

void pprocess_signal_pdesc_free(struct posix_process *process)
{
	struct uk_signal *sig;
	int signum;

	UK_ASSERT(process);
	UK_ASSERT(process->signal);

	pprocess_signal_foreach(signum)
		while ((sig = pprocess_signal_dequeue(process, NULL, signum)))
			uk_signal_free(process->_a, sig);

	pprocess_signal_sigaction_release(process->signal->sigaction);

	uk_free(process->_a, process->signal);
}

void pprocess_signal_tdesc_free(struct posix_thread *thread)
{
	struct posix_process *pproc;
	struct uk_signal *sig;
	int signum;

	UK_ASSERT(thread);
	UK_ASSERT(thread->signal);

	pproc = tid2pprocess(thread->tid);
	UK_ASSERT(pproc);

	pprocess_signal_foreach(signum)
		while ((sig = pprocess_signal_dequeue(pproc, thread, signum)))
			uk_signal_free(thread->_a, sig);

	uk_signal_free(thread->process->_a, thread->signal);
}
