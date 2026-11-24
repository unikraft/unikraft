/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/errptr.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>
#include <uk/paging.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/vmem.h>

#include "../process.h"

struct process_vas_probe {
	unsigned int releases;
	int cleanup_rc;
	bool inactive;
};

struct process_space {
	struct uk_vas vas;
	struct uk_pagetable pt;
	struct process_vas_probe *probe;
	struct uk_alloc *a;
};

static void process_vas_release(struct uk_vas *vas)
{
	struct process_space *s = __containerof(vas,
						   struct process_space, vas);
	struct process_vas_probe *p = s->probe;

	p->inactive = uk_vas_get_active() != vas;
	uk_vas_destroy(vas);
	p->cleanup_rc = uk_paging_pt_free(&s->pt, 0);
	if (!p->cleanup_rc)
		uk_free(s->a, s);
	p->releases++;
}

static struct process_space *process_vas_create(struct process_vas_probe *p)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct process_space *s;
	int rc;

	s = uk_malloc(kernel->a, sizeof(*s));
	if (!s)
		return NULL;
	memset_isr(s, 0, sizeof(*s));
	s->a = kernel->a;
	s->probe = p;
	rc = uk_paging_pt_clone_shared(&s->pt, kernel->pt, 0, span);
	if (rc)
		goto free_space;
	rc = uk_vas_init_owned(&s->vas, &s->pt, kernel->a,
			       process_vas_release);
	if (!rc)
		return s;
	uk_paging_pt_free(&s->pt, 0);
free_space:
	uk_free(kernel->a, s);
	return NULL;
}

static struct uk_thread *process_vas_thread(void)
{
	struct uk_sched *s = uk_sched_current();

	return uk_thread_create_container(s->a, s->a_stack, 0,
					  s->a_auxstack, 0, s->a_uktls, false,
					  "process-vas", NULL, NULL);
}

struct process_vas_allocator {
	struct uk_alloc *real;
	unsigned int calls;
	unsigned int freed;
};

static void *process_vas_malloc(struct uk_alloc *a, __sz size)
{
	struct process_vas_allocator *p =
		(struct process_vas_allocator *)a->priv;

	if (++p->calls == 2)
		return NULL;
	return uk_malloc(p->real, size);
}

static void *process_vas_calloc(struct uk_alloc *a, __sz count, __sz size)
{
	void *p;

	if (size && count > ((__sz)-1) / size)
		return NULL;
	p = process_vas_malloc(a, count * size);
	if (p)
		memset_isr(p, 0, count * size);
	return p;
}

static void process_vas_free(struct uk_alloc *a, void *ptr)
{
	struct process_vas_allocator *p =
		(struct process_vas_allocator *)a->priv;

	p->freed++;
	uk_free(p->real, ptr);
}

UK_TESTCASE(process_vas, retain_through_reap)
{
	struct process_vas_probe probe = { 0 };
	struct uk_thread *current = uk_thread_current();
	struct uk_vas *previous = current->vas;
	struct process_space *s;
	struct posix_process *process;
	struct posix_thread *pthread;
	struct uk_thread *thread;
	pid_t pid;
	int rc;

	UK_TEST_EXPECT(uk_vas_get_active() == uk_vas_get_kernel() &&
		       (!previous || previous == uk_vas_get_kernel()));
	if (uk_vas_get_active() != uk_vas_get_kernel() ||
	    (previous && previous != uk_vas_get_kernel()))
		return;
	s = process_vas_create(&probe);
	UK_TEST_EXPECT_NOT_NULL(s);
	if (!s)
		return;
	thread = process_vas_thread();
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto put_creator;
	rc = uk_thread_set_vas(current, &s->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto release_thread;
	process = pprocess_create(uk_sched_current()->a, thread, NULL);
	UK_TEST_EXPECT(!PTRISERR(process));
	if (PTRISERR(process))
		goto restore_current;
	UK_TEST_EXPECT_PTR_EQ(process->vas, &s->vas);
	UK_TEST_EXPECT_PTR_EQ(thread->vas_ref, &s->vas);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&s->vas.refs), 4);
	pid = process->pid;
	UK_TEST_EXPECT_PTR_EQ(pid2pprocess(pid), process);
	rc = uk_thread_set_vas(current, previous);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		UK_CRASH("Cannot restore process VAS test context");
	uk_vas_put(&s->vas);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&s->vas.refs), 2);
	pthread = uk_thread_uktls_var(thread, pthread_self);
	pprocess_release_pthread(pthread);
	pprocess_release(process);
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&s->vas.refs), 1);
	UK_TEST_EXPECT_ZERO(probe.releases);
	uk_thread_release(thread);
	UK_TEST_EXPECT_SNUM_EQ(probe.releases, 1);
	UK_TEST_EXPECT_ZERO(probe.cleanup_rc);
	UK_TEST_EXPECT(probe.inactive);
	return;
restore_current:
	rc = uk_thread_set_vas(current, previous);
	if (rc)
		UK_CRASH("Cannot restore failed process VAS test context");
release_thread:
	uk_thread_release(thread);
put_creator:
	uk_vas_put(&s->vas);
}

UK_TESTCASE(process_vas, creation_failure_drops_reference)
{
	struct process_vas_probe probe = { 0 };
	struct uk_alloc *real = uk_sched_current()->a;
	struct process_vas_allocator *allocations;
	struct uk_alloc *allocator;
	struct uk_thread *current = uk_thread_current();
	struct uk_vas *previous = current->vas;
	struct process_space *s;
	struct posix_process *process;
	struct uk_thread *thread;
	int rc;

	UK_TEST_EXPECT(uk_vas_get_active() == uk_vas_get_kernel() &&
		       (!previous || previous == uk_vas_get_kernel()));
	if (uk_vas_get_active() != uk_vas_get_kernel() ||
	    (previous && previous != uk_vas_get_kernel()))
		return;
	allocator = uk_malloc(real, sizeof(*allocator) + sizeof(*allocations));
	UK_TEST_EXPECT_NOT_NULL(allocator);
	if (!allocator)
		return;
	memset_isr(allocator, 0, sizeof(*allocator) + sizeof(*allocations));
	allocator->malloc = process_vas_malloc;
	allocator->calloc = process_vas_calloc;
	allocator->free = process_vas_free;
	allocations = (struct process_vas_allocator *)allocator->priv;
	allocations->real = real;
	s = process_vas_create(&probe);
	UK_TEST_EXPECT_NOT_NULL(s);
	if (!s)
		goto free_allocator;
	thread = process_vas_thread();
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto put_creator;
	rc = uk_thread_set_vas(current, &s->vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto release_thread;
	/* Fail the process signal allocation after acquiring its VAS ref. */
	process = pprocess_create(allocator, thread, NULL);
	UK_TEST_EXPECT_SNUM_EQ(PTR2ERR(process), -ENOMEM);
	UK_TEST_EXPECT_SNUM_EQ(allocations->calls, 2);
	UK_TEST_EXPECT_SNUM_EQ(allocations->freed, 1);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&s->vas.refs), 2);
	UK_TEST_EXPECT_ZERO(probe.releases);
	UK_TEST_EXPECT_NULL(uk_thread_uktls_var(thread, pthread_self));
	UK_TEST_EXPECT_NULL(thread->vas_ref);
	rc = uk_thread_set_vas(current, previous);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		UK_CRASH("Cannot restore failed process VAS context");
release_thread:
	uk_thread_release(thread);
put_creator:
	uk_vas_put(&s->vas);
	UK_TEST_EXPECT_SNUM_EQ(probe.releases, 1);
	UK_TEST_EXPECT_ZERO(probe.cleanup_rc);
	UK_TEST_EXPECT(probe.inactive);
free_allocator:
	uk_free(real, allocator);
}

_UK_TESTSUITE(process_vas, NULL);

static void __constructor run_process_vas(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_vas));
}
