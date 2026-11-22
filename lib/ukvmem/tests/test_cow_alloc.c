/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem.h>

#include "../cow.h"
#include "../cow_alloc.h"

#define COW_ALLOC_TEST_SIZE 128
#define COW_ALLOC_TEST_OBJECTS 160

struct cow_alloc_parent {
	struct uk_alloc *real;
	unsigned long allocated;
	unsigned long freed;
	bool fail;
};

static void *cow_alloc_test_pages(struct uk_alloc *a, unsigned long pages)
{
	struct cow_alloc_parent *p = (void *)a->priv;
	void *memory;

	if (__atomic_load_n(&p->fail, __ATOMIC_ACQUIRE))
		return NULL;
	memory = uk_palloc(p->real, pages);
	if (memory)
		__atomic_add_fetch(&p->allocated, pages, __ATOMIC_RELAXED);
	return memory;
}

static void cow_alloc_test_free(struct uk_alloc *a, void *memory,
				unsigned long pages)
{
	struct cow_alloc_parent *p = (void *)a->priv;

	uk_pfree(p->real, memory, pages);
	__atomic_add_fetch(&p->freed, pages, __ATOMIC_RELEASE);
}

static struct uk_alloc *cow_alloc_test_parent(struct uk_alloc *real)
{
	struct cow_alloc_parent *p;
	struct uk_alloc *a = uk_palloc(real, 1);

	if (!a)
		return NULL;
	UK_ASSERT(sizeof(*a) + sizeof(*p) <= UK_PAGING_PAGE_SIZE);
	memset_isr(a, 0, sizeof(*a) + sizeof(*p));
	a->palloc = cow_alloc_test_pages;
	a->pfree = cow_alloc_test_free;
	p = (void *)a->priv;
	p->real = real;
	return a;
}

UK_TESTCASE(cow_metadata, reuse_failure_and_reclaim)
{
	struct uk_alloc *real = uk_vas_get_active()->a;
	struct uk_alloc *registered = _uk_alloc_head;
	struct uk_alloc *parent, *a;
	struct cow_alloc_parent *p;
	struct vmem_cow_alloc_stats stats;
	void *objects[COW_ALLOC_TEST_OBJECTS] = { 0 };
	unsigned long irqf, allocated;
	unsigned int count, i;
	bool irq_off;
	void *extra;

	parent = cow_alloc_test_parent(real);
	UK_TEST_EXPECT_NOT_NULL(parent);
	if (!parent)
		return;
	p = (void *)parent->priv;
	p->fail = true;
	UK_TEST_EXPECT_NULL(vmem_cow_alloc_create(parent, COW_ALLOC_TEST_SIZE));
	UK_TEST_EXPECT_ZERO(p->allocated);
	p->fail = false;
	a = vmem_cow_alloc_create(parent, COW_ALLOC_TEST_SIZE);
	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		goto free_parent;
	UK_TEST_EXPECT_PTR_EQ(_uk_alloc_head, registered);
	UK_TEST_EXPECT_NULL(uk_malloc(a, COW_ALLOC_TEST_SIZE + 1));
	for (count = 0; count < 96; count++) {
		objects[count] = uk_malloc(a, COW_ALLOC_TEST_SIZE);
		if (!objects[count])
			break;
		UK_TEST_EXPECT(IS_ALIGNED((__uptr)objects[count], 16));
		*(__u64 *)objects[count] = count + 1;
	}
	UK_TEST_EXPECT_SNUM_EQ(count, 96);
	if (count != 96)
		goto free_objects;
	vmem_cow_alloc_stats(a, &stats);
	UK_TEST_EXPECT_SNUM_EQ(stats.objects, 96);
	UK_TEST_EXPECT(stats.slabs < 8);
	UK_TEST_EXPECT_SNUM_EQ(vmem_cow_alloc_destroy(a), -EBUSY);
	irqf = uk_lcpu_save_irqf();
	extra = uk_malloc(a, COW_ALLOC_TEST_SIZE);
	uk_free(a, extra);
	irq_off = uk_lcpu_irqs_disabled();
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_NOT_NULL(extra);
	UK_TEST_EXPECT(irq_off);
	allocated = p->allocated;
	p->fail = true;
	while (count < COW_ALLOC_TEST_OBJECTS) {
		objects[count] = uk_malloc(a, COW_ALLOC_TEST_SIZE);
		if (!objects[count])
			break;
		*(__u64 *)objects[count] = count + 1;
		count++;
	}
	UK_TEST_EXPECT(count < COW_ALLOC_TEST_OBJECTS);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, allocated);
	for (i = 0; i < count; i += 2) {
		uk_free(a, objects[i]);
		objects[i] = uk_malloc(a, COW_ALLOC_TEST_SIZE);
		UK_TEST_EXPECT_NOT_NULL(objects[i]);
		if (objects[i])
			*(__u64 *)objects[i] = i + 1;
	}
	for (i = 0; i < count; i++)
		if (objects[i])
			UK_TEST_EXPECT_SNUM_EQ(*(__u64 *)objects[i], i + 1);
free_objects:
	for (i = 0; i < count; i++)
		uk_free(a, objects[i]);
	vmem_cow_alloc_stats(a, &stats);
	UK_TEST_EXPECT_ZERO(stats.objects);
	UK_TEST_EXPECT_ZERO(stats.slabs);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, 1);
	UK_TEST_EXPECT_ZERO(vmem_cow_alloc_destroy(a));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, p->freed);
	UK_TEST_EXPECT_PTR_EQ(_uk_alloc_head, registered);
free_parent:
	uk_pfree(real, parent, 1);
}

struct cow_alloc_ref {
	struct vmem_cow_ref ref;
	struct uk_alloc *a;
};

static void cow_alloc_ref_free(void *arg)
{
	struct cow_alloc_ref *ref = arg;

	uk_free(ref->a, ref);
}

static struct cow_alloc_ref *cow_alloc_test_ref(struct uk_alloc *a)
{
	struct cow_alloc_ref *ref = uk_malloc(a, sizeof(*ref));

	if (ref) {
		memset_isr(ref, 0, sizeof(*ref));
		ref->a = a;
	}
	return ref;
}

UK_TESTCASE(cow_metadata, retirement_reclaims_last_slab)
{
	struct uk_vas *kernel = uk_vas_get_active();
	struct uk_alloc *parent, *a;
	struct cow_alloc_parent *p;
	struct cow_alloc_ref *first, *second;
	struct vmem_cow_alloc_stats stats;
	struct uk_paging_free_batch batch;
	int rc;

	parent = cow_alloc_test_parent(kernel->a);
	UK_TEST_EXPECT_NOT_NULL(parent);
	if (!parent)
		return;
	p = (void *)parent->priv;
	a = vmem_cow_alloc_create(parent, COW_ALLOC_TEST_SIZE);
	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		goto free_parent;
	first = cow_alloc_test_ref(a);
	UK_TEST_EXPECT_NOT_NULL(first);
	if (!first)
		goto destroy;
	rc = vmem_cow_ref_alloc(&first->ref, a, kernel->pt->fa,
				cow_alloc_ref_free, first);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_free(a, first);
		goto destroy;
	}
	second = cow_alloc_test_ref(a);
	UK_TEST_EXPECT_NOT_NULL(second);
	if (!second) {
		vmem_cow_ref_release(&first->ref, NULL);
		goto destroy;
	}
	vmem_cow_ref_clone(&second->ref, &first->ref,
			   cow_alloc_ref_free, second);
	uk_paging_free_batch_init(&batch, kernel->pt);
	vmem_cow_ref_release(&second->ref, &batch);
	vmem_cow_ref_release(&first->ref, NULL);
	vmem_cow_alloc_stats(a, &stats);
	UK_TEST_EXPECT_SNUM_EQ(stats.objects, 2);
	UK_TEST_EXPECT_SNUM_EQ(stats.slabs, 1);
	uk_paging_free_batch_finish(&batch);
	vmem_cow_alloc_stats(a, &stats);
	UK_TEST_EXPECT_ZERO(stats.objects);
	UK_TEST_EXPECT_ZERO(stats.slabs);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, 1);
destroy:
	UK_TEST_EXPECT_ZERO(vmem_cow_alloc_destroy(a));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, p->freed);
free_parent:
	uk_pfree(kernel->a, parent, 1);
}

struct cow_alloc_worker {
	struct uk_alloc *a;
	struct uk_semaphore *start;
	struct uk_semaphore *done;
	unsigned int id;
	bool failed;
};

static void cow_alloc_worker_gc(struct uk_thread *t __unused, void *arg)
{
	struct cow_alloc_worker *p = arg;

	uk_semaphore_up(p->done);
}

static void cow_alloc_worker_run(void *arg) __noreturn;

static void cow_alloc_worker_run(void *arg)
{
	struct cow_alloc_worker *p = arg;
	__u64 *objects[8];
	unsigned int round, i, count;
	__u64 value;

	uk_semaphore_down(p->start);
	for (round = 0; round < 32; round++) {
		for (count = 0; count < 8; count++) {
			objects[count] = uk_malloc(p->a, COW_ALLOC_TEST_SIZE);
			if (!objects[count]) {
				p->failed = true;
				break;
			}
			value = ((__u64)p->id << 32) | (round << 8) | count;
			objects[count][0] = value;
			objects[count][15] = ~value;
		}
		uk_sched_yield();
		for (i = 0; i < count; i++) {
			value = ((__u64)p->id << 32) | (round << 8) | i;
			if (objects[i][0] != value || objects[i][15] != ~value)
				p->failed = true;
			uk_free(p->a, objects[i]);
		}
		if (p->failed)
			break;
	}
	uk_sched_thread_exit2(cow_alloc_worker_gc, p);
}

UK_TESTCASE(cow_metadata, concurrent_reuse)
{
	struct uk_sched *sched = uk_sched_current();
	struct uk_alloc *real = uk_vas_get_active()->a;
	struct uk_alloc *parent, *a;
	struct cow_alloc_parent *p;
	struct cow_alloc_worker workers[8] = { 0 };
	struct uk_semaphore start, done;
	struct vmem_cow_alloc_stats stats;
	struct uk_thread *thread;
	unsigned int planned = MIN(2 * uk_sched_started_count(), 8U);
	unsigned int started = 0, i;
	const __nsec timeout = 2000000000ULL;
	__nsec elapsed;
	int rc;

	parent = cow_alloc_test_parent(real);
	UK_TEST_EXPECT_NOT_NULL(parent);
	if (!parent)
		return;
	p = (void *)parent->priv;
	a = vmem_cow_alloc_create(parent, COW_ALLOC_TEST_SIZE);
	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		goto free_parent;
	uk_semaphore_init(&start, 0);
	uk_semaphore_init(&done, 0);
	for (i = 0; i < planned; i++) {
		workers[i].a = a;
		workers[i].start = &start;
		workers[i].done = &done;
		workers[i].id = i + 1;
		thread = uk_thread_create_fn1(sched->a, cow_alloc_worker_run,
					      &workers[i], sched->a_stack, 0,
					      sched->a_auxstack, 0,
					      sched->a_uktls, false,
					      "cow-allocator", NULL, NULL);
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			break;
		rc = uk_sched_thread_add(uk_sched_select_round_robin(sched),
					 thread);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc) {
			uk_thread_release(thread);
			break;
		}
		started++;
	}
	UK_TEST_EXPECT_SNUM_EQ(started, planned);
	for (i = 0; i < started; i++)
		uk_semaphore_up(&start);
	for (i = 0; i < started; i++) {
		elapsed = uk_semaphore_down_to(&done, timeout);
		UK_TEST_EXPECT(elapsed < timeout / 2);
		if (elapsed >= timeout)
			UK_CRASH("COW allocator worker did not finish");
	}
	for (i = 0; i < started; i++)
		UK_TEST_EXPECT(!workers[i].failed);
	vmem_cow_alloc_stats(a, &stats);
	UK_TEST_EXPECT_ZERO(stats.objects);
	UK_TEST_EXPECT_ZERO(stats.slabs);
	UK_TEST_EXPECT_SNUM_EQ(p->allocated - p->freed, 1);
	UK_TEST_EXPECT_ZERO(vmem_cow_alloc_destroy(a));
	UK_TEST_EXPECT_SNUM_EQ(p->allocated, p->freed);
free_parent:
	uk_pfree(real, parent, 1);
}

_UK_TESTSUITE(cow_metadata, NULL);

static void __constructor run_cow_metadata(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(cow_metadata));
}
