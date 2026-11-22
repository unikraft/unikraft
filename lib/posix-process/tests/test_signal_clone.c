/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/crash.h>
#include <uk/errptr.h>
#include <uk/isr/string.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/test.h>

#include "../signal/signal.h"

struct clone_alloc_probe {
	struct uk_alloc *real;
	unsigned int allocated;
	unsigned int freed;
	unsigned int attempts;
	unsigned int fail_at;
};

struct clone_state {
	struct posix_process process;
	struct posix_thread thread;
};

static void *clone_probe_malloc(struct uk_alloc *a, __sz size)
{
	struct clone_alloc_probe *p = (void *)a->priv;
	void *memory;

	if (++p->attempts == p->fail_at)
		return NULL;
	memory = uk_malloc(p->real, size);

	if (memory)
		p->allocated++;
	return memory;
}

static void clone_probe_free(struct uk_alloc *a, void *memory)
{
	struct clone_alloc_probe *p = (void *)a->priv;

	p->freed++;
	uk_free(p->real, memory);
}

static struct uk_alloc *clone_probe_allocator(struct uk_alloc *real)
{
	struct clone_alloc_probe *p;
	struct uk_alloc *a = uk_malloc(real, sizeof(*a) + sizeof(*p));

	if (!a)
		UK_CRASH("Cannot allocate signal clone test interface\n");
	memset_isr(a, 0, sizeof(*a) + sizeof(*p));
	a->malloc = clone_probe_malloc;
	a->calloc = uk_calloc_compat;
	a->free = clone_probe_free;
	p = (void *)a->priv;
	p->real = real;
	return a;
}

static void clone_state_init(struct clone_state *s, struct uk_alloc *a,
			     pid_t pid)
{
	memset_isr(s, 0, sizeof(*s));
	s->process._a = a;
	s->process.pid = pid;
	UK_INIT_LIST_HEAD(&s->process.threads);
	s->thread._a = a;
	s->thread.process = &s->process;
	if (pprocess_signal_pdesc_alloc(&s->process) ||
	    pprocess_signal_pdesc_init(&s->process) ||
	    pprocess_signal_tdesc_alloc(&s->thread) ||
	    pprocess_signal_tdesc_init(&s->thread))
		UK_CRASH("Cannot initialize signal clone test state\n");
}

static void clone_state_free(struct clone_state *s)
{
	pprocess_signal_tdesc_free(&s->thread);
	pprocess_signal_pdesc_free(&s->process);
}

static void clone_queue_one(struct clone_state *s, bool thread, int signum)
{
	struct uk_signal_queue *q = thread ? &s->thread.signal->sigqueue :
					    &s->process.signal->sigqueue;
	struct uk_signal *sig = uk_signal_alloc(s->process._a);

	if (!sig)
		UK_CRASH("Cannot allocate queued signal for clone test\n");
	sig->siginfo.si_signo = signum;
	uk_list_add_tail(&sig->list_head, &q->list_head[signum]);
	uk_sigaddset(&q->pending, signum);
	s->process.signal->queued_count++;
}

UK_TESTCASE(signal_clone, private_table_preserves_owner)
{
	struct uk_alloc *real = uk_sched_current()->a;
	struct uk_alloc *a = clone_probe_allocator(real);
	struct clone_alloc_probe *p = (void *)a->priv;
	struct clone_state parent, child;
	struct uk_signal_pdesc *pd;
	struct uk_signal_tdesc *td;
	struct uk_sigaction *actions;
	long parent_refs;
	int rc;

	clone_state_init(&parent, real, 1);
	clone_state_init(&child, a, 2);
	pd = child.process.signal;
	td = child.thread.signal;
	actions = pd->sigaction;
	parent.process.signal->sigaction->ks[SIGUSR1].ks_handler = SIG_IGN;
	SET_MASKED(&parent.thread, SIGUSR2);
	SET_MASKED(&child.thread, SIGUSR1);
	clone_queue_one(&parent, false, SIGTERM);
	clone_queue_one(&parent, true, SIGINT);
	pprocess_signal_sigaction_acquire(parent.process.signal->sigaction);
	clone_queue_one(&child, false, SIGUSR1);
	clone_queue_one(&child, true, SIGUSR2);
	rc = pprocess_signal_clone(&child.thread, &parent.thread, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_PTR_EQ(child.thread.signal, td);
	UK_TEST_EXPECT_PTR_EQ(child.process.signal, pd);
	UK_TEST_EXPECT_PTR_EQ(pd->sigaction, actions);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&actions->refcnt), 1);
	UK_TEST_EXPECT_PTR_EQ(actions->alloc, a);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, 5);
	UK_TEST_EXPECT_ZERO(p->freed);
	UK_TEST_EXPECT(actions->ks[SIGUSR1].ks_handler == SIG_IGN);
	UK_TEST_EXPECT_SNUM_EQ(pd->queued_count, 2);
	UK_TEST_EXPECT(IS_PENDING(pd->sigqueue, SIGUSR1));
	UK_TEST_EXPECT(IS_PENDING(td->sigqueue, SIGUSR2));
	UK_TEST_EXPECT(IS_MASKED(&child.thread, SIGUSR2));
	UK_TEST_EXPECT(!IS_MASKED(&child.thread, SIGUSR1));
	UK_TEST_EXPECT(!IS_PENDING(pd->sigqueue, SIGTERM));
	UK_TEST_EXPECT(!IS_PENDING(td->sigqueue, SIGINT));
	UK_TEST_EXPECT(IS_PENDING(parent.process.signal->sigqueue, SIGTERM));
	UK_TEST_EXPECT(IS_PENDING(parent.thread.signal->sigqueue, SIGINT));
	SET_MASKED(&child.thread, SIGTERM);
	UK_TEST_EXPECT(!IS_MASKED(&parent.thread, SIGTERM));
	parent_refs =
		uk_refcount_read(&parent.process.signal->sigaction->refcnt);
	UK_TEST_EXPECT_SNUM_EQ(parent_refs, 2);
	clone_state_free(&child);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, p->allocated);
	pprocess_signal_sigaction_release(parent.process.signal->sigaction);
	clone_state_free(&parent);
	uk_free(real, a);
	uk_pr_info("SIGNAL_CLONE_PASS private\n");
}

UK_TESTCASE(signal_clone, sharing_releases_default_table)
{
	struct uk_alloc *real = uk_sched_current()->a;
	struct uk_alloc *a = clone_probe_allocator(real);
	struct clone_alloc_probe *p = (void *)a->priv;
	struct clone_state parent, child;
	struct uk_signal_pdesc *pd;
	struct uk_signal_tdesc *td;
	long parent_refs;
	int rc;

	clone_state_init(&parent, real, 1);
	clone_state_init(&child, a, 2);
	pd = child.process.signal;
	td = child.thread.signal;
	SET_MASKED(&parent.thread, SIGUSR2);
	rc = pprocess_signal_clone(&child.thread, &parent.thread,
				   CLONE_VM | CLONE_SIGHAND);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_PTR_EQ(child.thread.signal, td);
	UK_TEST_EXPECT_PTR_EQ(child.process.signal, pd);
	UK_TEST_EXPECT_PTR_EQ(pd->sigaction, parent.process.signal->sigaction);
	UK_TEST_EXPECT(IS_MASKED(&child.thread, SIGUSR2));
	SET_MASKED(&parent.thread, SIGTERM);
	UK_TEST_EXPECT(!IS_MASKED(&child.thread, SIGTERM));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, 3);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, 1);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&pd->sigaction->refcnt), 2);
	clone_state_free(&child);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, p->allocated);
	parent_refs =
		uk_refcount_read(&parent.process.signal->sigaction->refcnt);
	UK_TEST_EXPECT_SNUM_EQ(parent_refs, 1);
	clone_state_free(&parent);
	uk_free(real, a);
	uk_pr_info("SIGNAL_CLONE_PASS shared\n");
}

UK_TESTCASE(signal_clone, thread_and_clear_dispositions)
{
	struct uk_alloc *real = uk_sched_current()->a;
	struct uk_alloc *a = clone_probe_allocator(real);
	struct clone_alloc_probe *p = (void *)a->priv;
	struct clone_state parent, child;
	struct posix_thread thread = { ._a = a };
	struct uk_signal_tdesc *td;
	struct uk_sigaction *actions;
	int rc;

	clone_state_init(&parent, a, 1);
	thread.process = &parent.process;
	if (pprocess_signal_tdesc_alloc(&thread) ||
	    pprocess_signal_tdesc_init(&thread))
		UK_CRASH("Cannot allocate signal clone test descriptor\n");
	td = thread.signal;
	SET_MASKED(&parent.thread, SIGUSR1);
	rc = pprocess_signal_clone(&thread, &parent.thread,
				   CLONE_VM | CLONE_SIGHAND | CLONE_THREAD);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_PTR_EQ(thread.signal, td);
	UK_TEST_EXPECT(IS_MASKED(&thread, SIGUSR1));
	SET_MASKED(&thread, SIGUSR2);
	UK_TEST_EXPECT(!IS_MASKED(&parent.thread, SIGUSR2));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, 4);
	pprocess_signal_tdesc_free(&thread);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, 1);
	clone_state_init(&child, a, 2);
	actions = child.process.signal->sigaction;
	actions->ks[SIGUSR1].ks_handler = SIG_IGN;
	rc = pprocess_signal_clone(&child.thread, &parent.thread,
				   CLONE_SIGHAND | CLONE_CLEAR_SIGHAND |
				   CLONE_VM);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);
	rc = pprocess_signal_clone(&child.thread, &parent.thread,
				   CLONE_SIGHAND);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);
	rc = pprocess_signal_clone(&child.thread, &parent.thread, CLONE_THREAD);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EINVAL);
	UK_TEST_EXPECT(actions->ks[SIGUSR1].ks_handler == SIG_IGN);
	rc = pprocess_signal_clone(&child.thread, &parent.thread,
				   CLONE_CLEAR_SIGHAND);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_PTR_EQ(child.process.signal->sigaction, actions);
	UK_TEST_EXPECT(actions->ks[SIGUSR1].ks_handler == SIG_DFL);
	UK_TEST_EXPECT(IS_MASKED(&child.thread, SIGUSR1));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, 7);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, 1);
	clone_state_free(&child);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, 4);
	clone_state_free(&parent);
	UK_TEST_EXPECT_SNUM_EQ(p->freed, p->allocated);
	uk_free(real, a);
	uk_pr_info("SIGNAL_CLONE_PASS thread_clear\n");
}

UK_TESTCASE(signal_clone, process_action_allocation_rollback)
{
	struct uk_sched *sched = uk_sched_current();
	struct uk_alloc *a = clone_probe_allocator(sched->a);
	struct clone_alloc_probe *p = (void *)a->priv;
	struct posix_process *result;
	struct uk_thread *thread;
	void *binding;

	thread = uk_thread_create_container(sched->a, sched->a_stack, 0,
					    sched->a_auxstack, 0,
					    sched->a_uktls, false,
					    "signal-rollback", NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto free_allocator;
	/* Process and descriptor succeed; its initial action table fails. */
	p->fail_at = 3;
	for (unsigned int i = 0; i < 32; i++) {
		p->attempts = 0;
		p->allocated = 0;
		p->freed = 0;
		result = pprocess_create(a, thread, NULL);
		UK_TEST_EXPECT_SNUM_EQ(PTR2ERR(result), -ENOMEM);
		if (!PTRISERR(result))
			UK_CRASH("Signal allocation rollback did not fail\n");
		binding = uk_thread_uktls_var(thread, pthread_self);
		UK_TEST_EXPECT_NULL(binding);
		UK_TEST_EXPECT_SNUM_EQ(p->attempts, 3);
		UK_TEST_EXPECT_SNUM_EQ(p->allocated, 2);
		UK_TEST_EXPECT_SNUM_EQ(p->freed, 2);
	}
	uk_thread_release(thread);
	uk_pr_info("SIGNAL_CLONE_PASS rollback rounds=32\n");
free_allocator:
	uk_free(sched->a, a);
}

UK_TESTCASE(signal_clone, coalesced_standard_signal_releases_storage)
{
	struct uk_alloc *real = uk_sched_current()->a;
	struct uk_alloc *a = clone_probe_allocator(real);
	struct clone_alloc_probe *p = (void *)a->priv;
	struct clone_state state;
	struct uk_signal_queue *queue;
	struct uk_signal *first;
	siginfo_t info = { .si_code = SI_QUEUE };
	unsigned int resident;
	int rc;

	clone_state_init(&state, a, 2);
	queue = &state.process.signal->sigqueue;
	resident = p->allocated;
	for (unsigned int i = 0; i < 64; i++) {
		info.si_value.sival_int = i + 17;
		rc = pprocess_signal_send(&state.process, SIGUSR1, &info);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(state.process.signal->queued_count, 1);
		UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, resident + 1);
		first = uk_list_first_entry(&queue->list_head[SIGUSR1],
					    struct uk_signal, list_head);
		UK_TEST_EXPECT_SNUM_EQ(first->siginfo.si_value.sival_int, 17);
	}
	/* Real-time signals retain separate queue entries. */
	for (unsigned int i = 0; i < 2; i++) {
		rc = pprocess_signal_send(&state.process, SIGRTMIN, &info);
		UK_TEST_EXPECT_ZERO(rc);
	}
	UK_TEST_EXPECT_SNUM_EQ(state.process.signal->queued_count, 3);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, resident + 3);
	state.process.signal->queued_max = 3;
	rc = pprocess_signal_send(&state.process, SIGUSR2, &info);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EAGAIN);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, resident + 3);
	clone_state_free(&state);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, p->freed);
	uk_free(real, a);
	uk_pr_info("SIGNAL_COALESCE_PASS repeats=64 realtime=2\n");
}

_UK_TESTSUITE(signal_clone, NULL);

static void __constructor run_signal_clone(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(signal_clone));
}
