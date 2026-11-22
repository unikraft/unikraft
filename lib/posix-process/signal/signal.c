/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/plat/time.h>
#include <uk/process.h>
#include <uk/refcount.h>
#include <uk/sched.h>
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

static void signal_wake_waiter(struct posix_thread *thread, int signum)
{
	if (thread->state == POSIX_THREAD_BLOCKED_SIGWAIT) {
		if (uk_sigismember(&thread->signal->sigwait_set, signum) ||
		    (!IS_MASKED(thread, signum) &&
		     !IS_IGNORED(thread->process, signum)))
			uk_thread_wake(thread->thread);
	} else if (thread->state == POSIX_THREAD_BLOCKED_SIGNAL &&
		   !IS_MASKED(thread, signum) &&
		   !IS_IGNORED(thread->process, signum)) {
		uk_semaphore_up(&thread->signal->deliver_semaphore);
	}
}

/* Enqueue signal for a thread or a process
 *
 * Setting pthread to NULL signifies that the signal
 * targets the process. Success consumes sig, including coalesced signals;
 * on error the caller retains ownership.
 */
int pprocess_signal_queue_locked(struct posix_process *pproc,
				 struct posix_thread *pthread,
				 struct uk_signal *sig, bool *queued)
{
	struct uk_signal_queue *queue;
	int signum;

	UK_ASSERT(sig && pproc && queued);
	if (pthread)
		UK_ASSERT(pthread->process == pproc);
	*queued = false;
	signum = sig->siginfo.si_signo;
	queue = pthread ? &pthread->signal->sigqueue : &pproc->signal->sigqueue;
	/* Coalescing needs no capacity, even when real-time queues are full. */
	if (signum < SIGRTMIN && IS_PENDING(*queue, signum))
		goto notify;
	if (unlikely(pproc->signal->queued_count >= pproc->signal->queued_max))
		return -EAGAIN;
	uk_list_add_tail(&sig->list_head, &queue->list_head[signum]);
	__atomic_or_fetch(&queue->pending, (1UL << (signum - 1)),
			  __ATOMIC_RELEASE);
	pproc->signal->queued_count++;
	*queued = true;
notify:
	/* Keep each waiter linked for the duration of its wakeup. */
	if (pthread) {
		signal_wake_waiter(pthread, signum);
	} else {
		struct posix_thread *thread, *threadn;

		uk_pprocess_foreach_pthread(pproc, thread, threadn)
			signal_wake_waiter(thread, signum);
	}
	return 0;
}

/* Success consumes sig, including a coalesced signal; error retains it. */
static int pprocess_signal_enqueue(struct posix_process *pproc,
				   struct posix_thread *pthread,
				   struct uk_signal *sig)
{
	unsigned long flags;
	bool queued;
	int rc;

	flags = pprocess_lock_acquire();
	rc = pprocess_signal_queue_locked(pproc, pthread, sig, &queued);
	pprocess_lock_release(flags);
	if (!rc && !queued)
		uk_signal_free(sig->alloc, sig);
	return rc;
}

static struct uk_signal *signal_dequeue_locked(struct posix_process *pproc,
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
		return NULL;

	uk_list_del(&sig->list_head);

	/* Reset pending bit if the signal queue is empty */
	if (uk_list_empty(&sigqueue->list_head[signum])) {
		__atomic_and_fetch(&sigqueue->pending,
				   ~(1UL << (signum - 1)), __ATOMIC_RELEASE);
	}

	pproc->signal->queued_count--;
#if CONFIG_LIBPOSIX_PROCESS_ITIMER
	if (!pthread && signum == SIGALRM)
		pprocess_itimer_dequeued_locked(pproc);
#endif
	return sig;
}

struct uk_signal *
pprocess_signal_dequeue(struct posix_process *pproc,
			struct posix_thread *pthread, int signum)
{
	unsigned long flags = pprocess_lock_acquire();
	struct uk_signal *sig;

	sig = signal_dequeue_locked(pproc, pthread, signum);
	pprocess_lock_release(flags);
	return sig;
}

static struct uk_signal *signal_dequeue_set_locked(struct posix_thread *thread,
						   uk_sigset_t set)
{
	struct posix_process *proc = thread->process;
	uk_sigset_t pending = thread->signal->sigqueue.pending & set;

	if (!pending) {
		pending = proc->signal->sigqueue.pending & set;
		thread = NULL;
	}
	if (!pending)
		return NULL;

	return signal_dequeue_locked(proc, thread, __builtin_ctzl(pending) + 1);
}

static void detach_pending_queue(struct posix_process *proc,
				 struct uk_signal_queue *queue, int signum,
				 struct uk_list_head *discard)
{
	struct uk_signal *sig;

	while (!uk_list_empty(&queue->list_head[signum])) {
		sig = uk_list_first_entry(&queue->list_head[signum],
					  struct uk_signal, list_head);
		uk_list_del(&sig->list_head);
		uk_list_add_tail(&sig->list_head, discard);
		UK_ASSERT(proc->signal->queued_count);
		proc->signal->queued_count--;
	}
	__atomic_and_fetch(&queue->pending, ~(1UL << (signum - 1)),
			   __ATOMIC_RELEASE);
}

void pprocess_signal_clear_pending_locked(struct posix_process *proc,
					  int signum,
					  struct uk_list_head *discard)
{
	struct posix_thread *thread, *next;

	detach_pending_queue(proc, &proc->signal->sigqueue, signum, discard);
	uk_pprocess_foreach_pthread(proc, thread, next)
		detach_pending_queue(proc, &thread->signal->sigqueue, signum,
				     discard);
}

void pprocess_signal_free_list(struct uk_list_head *discard)
{
	struct uk_signal *sig;

	while (!uk_list_empty(discard)) {
		sig = uk_list_first_entry(discard, struct uk_signal, list_head);
		uk_list_del(&sig->list_head);
		uk_signal_free(sig->alloc, sig);
	}
}

void pprocess_signal_clear_pending(struct posix_process *proc, int signum)
{
	struct uk_list_head discard;
	unsigned long flags;

	UK_INIT_LIST_HEAD(&discard);
	flags = pprocess_lock_acquire();
	pprocess_signal_clear_pending_locked(proc, signum, &discard);
	pprocess_lock_release(flags);
	pprocess_signal_free_list(&discard);
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

#if CONFIG_LIBPOSIX_PROCESS_SIGNALFD
	pprocess_signal_files_notify(proc, signum);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNALFD */

	return 0;
}

static bool signal_unmasked_pending_locked(struct posix_thread *thread)
{
	struct posix_process *proc = thread->process;
	uk_sigset_t pending = (thread->signal->sigqueue.pending |
			      proc->signal->sigqueue.pending);
	int signum;

	pending &= ~thread->signal->mask;
	while (pending) {
		signum = __builtin_ctzl(pending) + 1;
		if (!IS_IGNORED(proc, signum))
			return true;
		uk_sigdelset(&pending, signum);
	}
	return false;
}

void pprocess_signal_wait(struct posix_thread *thread)
{
	for (;;) {
		unsigned long pflags = pprocess_lock_acquire();

		if (signal_unmasked_pending_locked(thread)) {
			thread->state = POSIX_THREAD_RUNNING;
			pprocess_lock_release(pflags);
			return;
		}

		/* Publish the waiter under the same lock used to enqueue. */
		thread->state = POSIX_THREAD_BLOCKED_SIGNAL;
		pprocess_lock_release(pflags);
		uk_semaphore_down(&thread->signal->deliver_semaphore);
	}
}

int pprocess_signal_wait_set(struct posix_thread *thread, uk_sigset_t set,
			     __snsec deadline, siginfo_t *info)
{
	unsigned long flags;
	struct uk_signal *sig;
	int ret;

	uk_sigdelset(&set, SIGSTOP);
	uk_sigdelset(&set, SIGKILL);

	for (;;) {
		flags = pprocess_lock_acquire();
		sig = signal_dequeue_set_locked(thread, set);
		if (sig) {
			ret = sig->siginfo.si_signo;
			break;
		}
		if (signal_unmasked_pending_locked(thread)) {
			ret = -EINTR;
			break;
		}
		if (deadline && ukplat_monotonic_clock() >= (__nsec)deadline) {
			ret = -EAGAIN;
			break;
		}

		/* Publish and block under the lock used by signal enqueue. */
		thread->signal->sigwait_set = set;
		thread->state = POSIX_THREAD_BLOCKED_SIGWAIT;
		uk_thread_block_until(thread->thread, deadline);
		pprocess_lock_release(flags);
		uk_sched_yield();
	}

	thread->state = POSIX_THREAD_RUNNING;
	uk_sigemptyset(&thread->signal->sigwait_set);
	pprocess_lock_release(flags);
	if (sig) {
		if (info)
			*info = sig->siginfo;
		uk_signal_free(sig->alloc, sig);
	}
	return ret;
}

/* The selected process pin covers allocation, enqueue and FD notification. */
static int signal_process_pinned(struct posix_process *process, int signum,
				 siginfo_t *siginfo)
{
	struct uk_signal *signal;
	unsigned long flags;
	bool queued = false;
	int result = 0;

	if (signum == SIGSTOP || signum == SIGCONT)
		return 0;
	signal = uk_signal_alloc(process->_a);
	if (!signal)
		return -EAGAIN;
	if (siginfo)
		set_siginfo_sigqueue(signum, &signal->siginfo, siginfo);
	else
		set_siginfo_kill(signum, &signal->siginfo);
	flags = pprocess_lock_acquire();
	if (process->exiting || process->state != POSIX_PROCESS_RUNNING ||
	    pprocess_signal_should_drop(process, signum)) {
		pprocess_lock_release(flags);
		uk_signal_free(signal->alloc, signal);
		return 0;
	}
	result = pprocess_signal_queue_locked(process, NULL, signal, &queued);
	pprocess_lock_release(flags);
	if (!queued)
		uk_signal_free(signal->alloc, signal);
#if CONFIG_LIBPOSIX_PROCESS_SIGNALFD
	if (!result)
		pprocess_signal_files_notify(process, signum);
#endif
	return result;
}

int pprocess_signal_process_do(pid_t pid, int signum, siginfo_t *siginfo)
{
	struct posix_process *self = uk_pthread_current()->process;
	struct posix_process *process, *target = NULL, *single;
	struct posix_process **targets = &single;
	struct uk_alloc *alloc = uk_alloc_get_default();
	unsigned long flags;
	unsigned int count = 0, index;
	pid_t pgid = 0;
	bool many = pid <= 0 && signum;
	int result = -ESRCH, rc;

	if (signum && !IS_VALID(signum))
		return -EINVAL;
	if (pid < -CONFIG_LIBPOSIX_PROCESS_MAX_PID)
		return -ESRCH;
	if (many) {
		targets = uk_calloc(alloc, TIDMAP_SIZE, sizeof(*targets));
		if (!targets)
			return -ENOMEM;
	}
	flags = pprocess_lock_acquire();
	if (pid > 0) {
		target = pid2pprocess(pid);
		if (!target)
			target = tid2pprocess(pid);
	} else if (pid != -1) {
		pgid = pid ? -pid : self->pgid;
	}
	/* Snapshot and pin recipients while membership is stable. Signal zero
	 * only checks existence and permissions; it never queues an entry.
	 */
	uk_pprocess_foreach(process) {
		if (pid > 0 && process != target)
			continue;
		if (pgid && process->pgid != pgid)
			continue;
		if (pid == -1 &&
		    (process == self || process->pid == UK_PID_INIT))
			continue;
		if (!signal_check_perm(process)) {
			result = -EPERM;
			continue;
		}
		if (signum) {
			pprocess_get(process);
			targets[count] = process;
		}
		count++;
	}
	pprocess_lock_release(flags);
	if (!count)
		goto out;
	if (!signum) {
		result = 0;
		goto out;
	}
	if (signum == SIGSTOP || signum == SIGCONT)
		uk_pr_warn("Process stop / resume not supported. Ignoring\n");
	for (index = 0; index < count; index++) {
		rc = signal_process_pinned(targets[index], signum, siginfo);
		pprocess_put(targets[index]);
		if (!rc || result)
			result = rc;
	}
out:
	if (many)
		uk_free(alloc, targets);
	return result;
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

#if CONFIG_LIBPOSIX_PROCESS_SIGNALFD
	pthread_signal_files_notify(pproc, pthread, signum);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNALFD */

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

#if CONFIG_LIBPOSIX_PROCESS_SIGNALFD
	uk_signal_files_ctx_init(&pd->sigfiles_ctx);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNALFD */

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

	uk_sigemptyset(&td->sigwait_set);
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

	/* Initialization may have failed while allocating the action table. */
	if (process->signal->sigaction)
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

	pproc = thread->process;
	UK_ASSERT(pproc);

	pprocess_signal_foreach(signum)
		while ((sig = pprocess_signal_dequeue(pproc, thread, signum)))
			uk_signal_free(thread->_a, sig);

	uk_signal_free(thread->process->_a, thread->signal);
}
