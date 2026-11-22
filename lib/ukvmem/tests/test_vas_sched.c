/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/paging.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/vmem.h>

struct deferred_release_probe {
	struct uk_pagetable *pt;
	__vaddr_t address;
	unsigned int calls;
	bool irqs_enabled;
	bool read_only;
};

static void deferred_release(void *arg)
{
	struct deferred_release_probe *probe = arg;
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	__pte_t pte;
	int rc;

	probe->calls++;
	probe->irqs_enabled = !uk_lcpu_irqs_disabled();
	rc = uk_paging_pt_walk(probe->pt, probe->address, &level, NULL, &pte);
	probe->read_only = !rc &&
		!(uk_paging_attr_from_pte(pte, level) &
		  UK_PAGING_PAGE_ATTR_PROT_WRITE);
}

UK_TESTCASE(vas_sched, deferred_backing_release)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_paging_deferred_call calls[2];
	struct uk_paging_free_batch batch;
	struct deferred_release_probe probe = { .pt = kernel->pt };
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	int rc;

	rc = uk_vma_map_anon(kernel, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	probe.address = address;

	uk_paging_batch_begin(&batch, kernel->pt);
	uk_paging_batch_defer(&batch, &calls[0], deferred_release, &probe);
	uk_paging_batch_defer(&batch, &calls[1], deferred_release, &probe);
	rc = uk_paging_page_set_attr(kernel->pt, address, 1,
				     UK_PAGING_PAGE_ATTR_PROT_READ, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(probe.calls);
	uk_paging_batch_end(&batch);
	UK_TEST_EXPECT_ZERO(probe.calls);
	uk_paging_free_batch_finish(&batch);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, 2);
	UK_TEST_EXPECT(probe.irqs_enabled);
	UK_TEST_EXPECT(probe.read_only);

	/* A completed batch must not release the same references twice. */
	uk_paging_free_batch_finish(&batch);
	UK_TEST_EXPECT_SNUM_EQ(probe.calls, 2);

	rc = uk_vma_unmap(kernel, address, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
}

struct vas_switch_probe {
	struct uk_vas *vas;
	__vaddr_t address;
	bool done;
	bool first_active;
	bool restored;
};

static void vas_switch_thread(void *arg) __noreturn;

static void vas_switch_thread(void *arg)
{
	struct vas_switch_probe *probe = arg;
	__u64 *value = (__u64 *)probe->address;

	probe->first_active = uk_vas_get_active() == probe->vas &&
			      uk_paging_pt_get_active() == probe->vas->pt &&
			      __atomic_load_n(value, __ATOMIC_RELAXED) == 0;
	__atomic_store_n(value, 2, __ATOMIC_RELAXED);
	uk_sched_yield();
	probe->restored = uk_vas_get_active() == probe->vas &&
			  __atomic_load_n(value, __ATOMIC_RELAXED) == 2;
	__atomic_store_n(&probe->done, true, __ATOMIC_RELEASE);
	uk_thread_exit();
}

UK_TESTCASE(vas_sched, switch_distinct_address_spaces)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_sched *sched = uk_sched_current();
	struct uk_pagetable child_pt;
	struct uk_vas child;
	struct vas_switch_probe probe = { .vas = &child };
	struct uk_thread *thread;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__u64 root_value;
	int rc;

	UK_TEST_EXPECT(kernel && uk_vas_get_active() == kernel);
	if (!kernel || uk_vas_get_active() != kernel)
		return;

	/* Reserve the test address without mapping a frame. The snapshot
	 * must include the test thread's kernel stack, TLS, and metadata.
	 */
	rc = uk_vma_map_anon(kernel, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;

	thread = uk_thread_create_fn1(sched->a, vas_switch_thread, &probe,
				      sched->a_stack, 0, sched->a_auxstack, 0,
				      sched->a_uktls, false, "vas-switch",
				      NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto unmap_kernel;

	rc = uk_paging_pt_clone(&child_pt, kernel->pt, 0);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto release_thread;
	rc = uk_vas_init(&child, &child_pt, uk_alloc_get_default());
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;

	probe.address = address;
	rc = uk_vma_map_anon(&child, &probe.address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_child;
	UK_TEST_EXPECT(probe.address == address);
	__atomic_store_n((__u64 *)address, 1, __ATOMIC_RELAXED);

	thread->vas = &child;
	rc = uk_sched_thread_add(sched, thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_child;
	thread = NULL; /* The scheduler releases the test thread on exit. */

	for (unsigned int i = 0;
	     !__atomic_load_n(&probe.done, __ATOMIC_ACQUIRE) && i < 64; ++i)
		uk_sched_yield();
	if (unlikely(!__atomic_load_n(&probe.done, __ATOMIC_ACQUIRE)))
		UK_CRASH("Address-space test thread did not terminate\n");
	UK_TEST_EXPECT(probe.first_active);
	UK_TEST_EXPECT(probe.restored);
	UK_TEST_EXPECT(uk_vas_get_active() == kernel);
	root_value = __atomic_load_n((__u64 *)address, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_SNUM_EQ(root_value, 1);

destroy_child:
	uk_vas_destroy(&child);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&child_pt,
					      UK_PAGING_PAGE_FLAG_KEEP_FRAMES));
release_thread:
	if (thread)
		uk_thread_release(thread);
unmap_kernel:
	rc = uk_vma_unmap(kernel, address, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
}

_UK_TESTSUITE(vas_sched, NULL);

static void __constructor run_vas_sched(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(vas_sched));
}
