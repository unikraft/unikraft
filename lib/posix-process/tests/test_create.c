/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <uk/alloc.h>
#include <uk/errptr.h>
#include <uk/isr/string.h>
#include <uk/sched.h>
#include <uk/test.h>

#include "../process.h"

struct create_allocator {
	struct uk_alloc *real;
	unsigned int attempts;
	unsigned int allocated;
	unsigned int freed;
};

static void *create_malloc(struct uk_alloc *a, __sz size)
{
	struct create_allocator *probe = (struct create_allocator *)a->priv;
	void *p;

	/* The POSIX thread succeeds; its signal descriptor must fail. */
	if (++probe->attempts == 2)
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

_UK_TESTSUITE(process_create, NULL);

static void __constructor run_process_create(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_create));
}
