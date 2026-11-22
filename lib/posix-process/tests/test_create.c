/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <uk/alloc.h>
#include <uk/essentials.h>
#include <uk/errptr.h>
#include <uk/isr/string.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/test.h>

#include "../process.h"

struct create_allocator {
	struct uk_alloc *real;
	unsigned int attempts;
	unsigned int allocated;
	unsigned int freed;
	unsigned int fail_at;
};

static void *create_malloc(struct uk_alloc *a, __sz size)
{
	struct create_allocator *probe = (struct create_allocator *)a->priv;
	void *p;

	if (++probe->attempts == probe->fail_at)
		return NULL;
	p = uk_malloc(probe->real, size);
	if (p)
		probe->allocated++;
	return p;
}

static void *create_calloc(struct uk_alloc *a, __sz count, __sz size)
{
	void *p;

	if (size && count > ((__sz)-1) / size)
		return NULL;
	p = create_malloc(a, count * size);
	if (p)
		memset_isr(p, 0, count * size);
	return p;
}

static void create_free(struct uk_alloc *a, void *p)
{
	struct create_allocator *probe = (struct create_allocator *)a->priv;

	if (!p)
		return;
	probe->freed++;
	uk_free(probe->real, p);
}

static unsigned int create_thread_count(void)
{
	unsigned int count = 0;
	pid_t tid;

	for (tid = 1; tid <= CONFIG_LIBPOSIX_PROCESS_MAX_PID; tid++)
		if (tid2pthread(tid))
			count++;
	return count;
}

UK_TESTCASE(process_create, signal_allocation_rollback)
{
	struct uk_sched *sched = uk_sched_current();
	struct create_allocator *probe;
	struct uk_alloc *allocator;
	struct posix_process process = { 0 };
	struct posix_thread *pthread;
	struct uk_thread *thread;
	unsigned int before = create_thread_count();
	unsigned int rounds = CONFIG_LIBPOSIX_PROCESS_MAX_PID + 1;
	unsigned int i;
	void *binding;

	allocator = uk_malloc(sched->a, sizeof(*allocator) + sizeof(*probe));
	UK_TEST_EXPECT_NOT_NULL(allocator);
	if (!allocator)
		return;
	memset_isr(allocator, 0, sizeof(*allocator) + sizeof(*probe));
	allocator->malloc = create_malloc;
	allocator->calloc = create_calloc;
	allocator->free = create_free;
	probe = (struct create_allocator *)allocator->priv;
	probe->real = sched->a;
	/* The POSIX thread succeeds; its signal descriptor must fail. */
	probe->fail_at = 2;
	process._a = allocator;
	UK_INIT_LIST_HEAD(&process.threads);
	thread = uk_thread_create_container(sched->a, sched->a_stack, 0,
					    sched->a_auxstack, 0,
					    sched->a_uktls, false,
					    "process-create", NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto free_allocator;

	/* More attempts than the TID space also detect a leaked reservation. */
	for (i = 0; i < rounds; i++) {
		probe->attempts = 0;
		probe->allocated = 0;
		probe->freed = 0;
		pthread = pprocess_create_pthread(&process, thread);
		UK_TEST_EXPECT_SNUM_EQ(PTR2ERR(pthread), -ENOMEM);
		binding = uk_thread_uktls_var(thread, pthread_self);
		UK_TEST_EXPECT_NULL(binding);
		/* Do not pass a stale binding into termination. */
		if (binding) {
			uk_thread_uktls_var(thread, pthread_self) = NULL;
			uk_thread_release(thread);
			goto free_allocator;
		}
		UK_TEST_EXPECT_SNUM_EQ(probe->attempts, 2);
		UK_TEST_EXPECT_SNUM_EQ(probe->allocated, 1);
		UK_TEST_EXPECT_SNUM_EQ(probe->freed, 1);
		UK_TEST_EXPECT(uk_list_empty(&process.threads));
		UK_TEST_EXPECT_SNUM_EQ(create_thread_count(), before);
	}
	uk_thread_release(thread);
	UK_TEST_EXPECT_SNUM_EQ(create_thread_count(), before);
free_allocator:
	uk_free(sched->a, allocator);
}

static struct uk_thread *create_raw_thread(struct uk_sched *s)
{
	return uk_thread_create_container(s->a, s->a_stack, 0,
					   s->a_auxstack, 0, s->a_uktls, false,
					   "process-ids", NULL, NULL);
}

static unsigned int create_process_count(void)
{
	unsigned long flags = pprocess_lock_acquire();
	unsigned int count = 0;

	for (unsigned int i = 1; i < TIDMAP_SIZE; i++)
		count += !!pid_process[i];
	pprocess_lock_release(flags);
	return count;
}

static struct uk_alloc *create_tracking_allocator(void)
{
	struct uk_alloc *a;
	struct create_allocator *alloc;

	a = uk_malloc(uk_sched_current()->a, sizeof(*a) + sizeof(*alloc));
	if (!a)
		return NULL;
	memset_isr(a, 0, sizeof(*a) + sizeof(*alloc));
	a->malloc = create_malloc;
	a->calloc = create_calloc;
	a->free = create_free;
	alloc = (void *)a->priv;
	alloc->real = uk_sched_current()->a;
	return a;
}

UK_TESTCASE(process_create, pid_reservation_across_leader_exit)
{
	struct uk_sched *sched = uk_sched_current();
	struct create_allocator *alloc;
	struct uk_alloc *a;
	struct uk_thread *threads[4] = { 0 };
	struct posix_process *held = NULL, *owner = NULL;
	struct posix_thread *pt;
	unsigned int before_threads = create_thread_count();
	unsigned int before_processes = create_process_count();
	unsigned int rounds = CONFIG_LIBPOSIX_PROCESS_MAX_PID + 1;
	bool protected = true, reused = false;
	pid_t held_pid;

	a = create_tracking_allocator();
	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	for (unsigned int i = 0; i < ARRAY_SIZE(threads); i++) {
		threads[i] = create_raw_thread(sched);
		UK_TEST_EXPECT_NOT_NULL(threads[i]);
		if (!threads[i])
			goto release;
	}
	held = pprocess_create(a, threads[0], NULL);
	UK_TEST_EXPECT(!PTRISERR(held));
	if (PTRISERR(held)) {
		held = NULL;
		goto release;
	}
	owner = pprocess_create(a, threads[1], NULL);
	UK_TEST_EXPECT(!PTRISERR(owner));
	if (PTRISERR(owner)) {
		owner = NULL;
		goto release;
	}
	pt = pprocess_create_pthread(held, threads[2]);
	UK_TEST_EXPECT(!PTRISERR(pt));
	if (PTRISERR(pt))
		goto release;
	held_pid = held->pid;
	pt = uk_thread_uktls_var(threads[0], pthread_self);
	pprocess_release_pthread(pt);
	UK_TEST_EXPECT_NULL(tid2pthread(held_pid));
	/* Protect the PID both with a live sibling and with no thread left. */
	for (unsigned int phase = 0; phase < 2; phase++) {
		if (phase) {
			pt = uk_thread_uktls_var(threads[2], pthread_self);
			pprocess_release_pthread(pt);
		}
		for (unsigned int i = 0; i < rounds; i++) {
			pt = pprocess_create_pthread(owner, threads[3]);
			UK_TEST_EXPECT(!PTRISERR(pt));
			if (PTRISERR(pt))
				goto release;
			protected &= pt->tid != held_pid;
			UK_TEST_EXPECT(pt->tid != held_pid);
			UK_TEST_EXPECT_PTR_EQ(pid2pprocess(held_pid), held);
			pprocess_release_pthread(pt);
		}
	}
	pprocess_release(held);
	held = NULL;
	UK_TEST_EXPECT_NULL(pid2pprocess(held_pid));
	for (unsigned int i = 0; i < rounds; i++) {
		pt = pprocess_create_pthread(owner, threads[3]);
		UK_TEST_EXPECT(!PTRISERR(pt));
		if (PTRISERR(pt))
			goto release;
		reused |= pt->tid == held_pid;
		pprocess_release_pthread(pt);
	}
	UK_TEST_EXPECT(reused);
	uk_pr_info("PROCESS_IDS_RESULT rounds=%u protected=%u reused=%u\n",
		   rounds, protected, reused);
release:
	for (unsigned int i = 0; i < ARRAY_SIZE(threads); i++) {
		if (!threads[i])
			continue;
		pt = uk_thread_uktls_var(threads[i], pthread_self);
		if (pt)
			pprocess_release_pthread(pt);
	}
	if (held)
		pprocess_release(held);
	if (owner)
		pprocess_release(owner);
	for (unsigned int i = 0; i < ARRAY_SIZE(threads); i++)
		if (threads[i])
			uk_thread_release(threads[i]);
	UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
	UK_TEST_EXPECT_SNUM_EQ(create_thread_count(), before_threads);
	UK_TEST_EXPECT_SNUM_EQ(create_process_count(), before_processes);
	uk_free(sched->a, a);
}

struct create_pid_slot {
	struct uk_thread *thread;
	struct posix_process *process;
};

UK_TESTCASE(process_create, published_pid_exhaustion_and_recovery)
{
	struct uk_sched *sched = uk_sched_current();
	struct uk_alloc *a = create_tracking_allocator();
	struct create_allocator *alloc;
	struct create_pid_slot *slots = NULL;
	struct uk_thread *owner_thread = NULL, *probe_thread = NULL;
	struct posix_process *owner = NULL, *process;
	struct posix_thread *pt;
	unsigned int before_threads = create_thread_count();
	unsigned int before_processes = create_process_count();
	unsigned int held = 0, recovered = 0;
	bool filled = false, blocked = false, balanced;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	slots = uk_calloc(a, TIDMAP_SIZE, sizeof(*slots));
	UK_TEST_EXPECT_NOT_NULL(slots);
	if (!slots)
		goto release;
	owner_thread = create_raw_thread(sched);
	probe_thread = create_raw_thread(sched);
	UK_TEST_EXPECT(owner_thread && probe_thread);
	if (!owner_thread || !probe_thread)
		goto release;
	owner = pprocess_create(a, owner_thread, NULL);
	UK_TEST_EXPECT(!PTRISERR(owner));
	if (PTRISERR(owner)) {
		owner = NULL;
		goto release;
	}
	/* Fill with leaders first, so even a broken PID check cannot publish
	 * two processes under one number while preparing this test.
	 */
	for (unsigned int i = 0; i < TIDMAP_SIZE; i++) {
		slots[i].thread = create_raw_thread(sched);
		UK_TEST_EXPECT_NOT_NULL(slots[i].thread);
		if (!slots[i].thread)
			goto release;
		process = pprocess_create(a, slots[i].thread, NULL);
		if (PTRISERR(process)) {
			filled = PTR2ERR(process) == -EAGAIN;
			UK_TEST_EXPECT(filled);
			break;
		}
		slots[i].process = process;
		held++;
	}
	UK_TEST_EXPECT(filled && held);
	if (!filled || !held)
		goto release;
	for (unsigned int i = 0; i < held; i++) {
		pt = uk_thread_uktls_var(slots[i].thread, pthread_self);
		pprocess_release_pthread(pt);
		UK_TEST_EXPECT_PTR_EQ(pid2pprocess(slots[i].process->pid),
				      slots[i].process);
	}
	/* Only published PIDs, rather than their leader TIDs, now fill all
	 * remaining slots. Thread allocation must still report EAGAIN.
	 */
	pt = pprocess_create_pthread(owner, probe_thread);
	blocked = PTRISERR(pt) && PTR2ERR(pt) == -EAGAIN;
	UK_TEST_EXPECT(blocked);
	if (!PTRISERR(pt))
		pprocess_release_pthread(pt);
	for (unsigned int i = 0; i < held; i++) {
		pprocess_release(slots[i].process);
		slots[i].process = NULL;
	}
	/* Recover every former slot, not merely one successful allocation. */
	for (unsigned int i = 0; i < held; i++) {
		pt = pprocess_create_pthread(owner, slots[i].thread);
		UK_TEST_EXPECT(!PTRISERR(pt));
		if (PTRISERR(pt))
			goto release;
		recovered++;
	}
	UK_TEST_EXPECT_SNUM_EQ(recovered, held);
	pt = pprocess_create_pthread(owner, probe_thread);
	UK_TEST_EXPECT(PTRISERR(pt) && PTR2ERR(pt) == -EAGAIN);
	if (!PTRISERR(pt))
		pprocess_release_pthread(pt);
release:
	if (slots) {
		for (unsigned int i = 0; i < TIDMAP_SIZE; i++) {
			if (slots[i].thread) {
				struct uk_thread *thread = slots[i].thread;

				pt = uk_thread_uktls_var(thread, pthread_self);
				if (pt)
					pprocess_release_pthread(pt);
			}
			if (slots[i].process)
				pprocess_release(slots[i].process);
			if (slots[i].thread)
				uk_thread_release(slots[i].thread);
		}
		uk_free(a, slots);
	}
	if (probe_thread) {
		pt = uk_thread_uktls_var(probe_thread, pthread_self);
		if (pt)
			pprocess_release_pthread(pt);
		uk_thread_release(probe_thread);
	}
	if (owner_thread) {
		pt = uk_thread_uktls_var(owner_thread, pthread_self);
		if (pt)
			pprocess_release_pthread(pt);
		uk_thread_release(owner_thread);
	}
	if (owner)
		pprocess_release(owner);
	balanced = alloc->allocated == alloc->freed;
	UK_TEST_EXPECT(balanced);
	UK_TEST_EXPECT_SNUM_EQ(create_thread_count(), before_threads);
	UK_TEST_EXPECT_SNUM_EQ(create_process_count(), before_processes);
	uk_pr_info("PROCESS_IDS_EXHAUST_RESULT held=%u blocked=%u recovered=%u balanced=%u\n",
		   held, blocked, recovered, balanced);
	uk_free(sched->a, a);
}

_UK_TESTSUITE(process_create, NULL);

static void __constructor run_process_create(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_create));
}
