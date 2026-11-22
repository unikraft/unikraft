/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem.h>

#define VAS_REF_TIMEOUT ((__nsec)2000000000ULL)
#define VAS_REF_VALUE 0x6f776e6564766173ULL

struct vas_ref_probe {
	struct uk_semaphore start;
	struct uk_semaphore reaped;
	struct uk_semaphore released;
	struct uk_vas *vas;
	__vaddr_t address;
	__u64 value;
	unsigned int releases;
	int cleanup_rc;
	int term_rebind_rc;
	int dtor_rebind_rc;
	bool active;
	bool term_seen;
	bool term_vas_live;
	bool dtor_vas_live;
	bool gc_seen;
	bool dtor_seen;
	bool released_after_gc;
	bool released_after_term;
	bool released_after_dtor;
	bool released_inactive;
};

static const char vas_ref_worker_name[] = "vas-owned";

struct vas_ref_space {
	struct uk_vas vas;
	struct uk_pagetable pt;
	struct uk_alloc *a;
	struct vas_ref_probe *probe;
};

static void vas_ref_release(struct uk_vas *vas)
{
	struct vas_ref_space *space = __containerof(vas,
						   struct vas_ref_space, vas);
	struct vas_ref_probe *p = space->probe;
	struct uk_alloc *a = space->a;

	p->released_inactive = uk_vas_get_active() != vas;
	p->released_after_gc = __atomic_load_n(&p->gc_seen, __ATOMIC_ACQUIRE);
	p->released_after_term = __atomic_load_n(&p->term_seen,
						 __ATOMIC_ACQUIRE);
	p->released_after_dtor = __atomic_load_n(&p->dtor_seen,
						 __ATOMIC_ACQUIRE);
	uk_vas_destroy(vas);
	p->cleanup_rc = uk_paging_pt_free(&space->pt, 0);
	if (!p->cleanup_rc)
		uk_free(a, space);
	__atomic_add_fetch(&p->releases, 1, __ATOMIC_RELEASE);
	uk_semaphore_up(&p->released);
}

static struct vas_ref_space *vas_ref_create(struct vas_ref_probe *p)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct vas_ref_space *space;
	int rc;

	uk_semaphore_init(&p->start, 0);
	uk_semaphore_init(&p->reaped, 0);
	uk_semaphore_init(&p->released, 0);
	space = uk_malloc(kernel->a, sizeof(*space));
	if (!space)
		return NULL;
	memset_isr(space, 0, sizeof(*space));
	space->a = kernel->a;
	space->probe = p;
	rc = uk_paging_pt_clone_shared(&space->pt, kernel->pt, 0, span);
	if (rc)
		goto free_space;
	rc = uk_vas_init_owned(&space->vas, &space->pt, kernel->a,
			       vas_ref_release);
	if (rc) {
		uk_paging_pt_free(&space->pt, 0);
		goto free_space;
	}
	p->vas = &space->vas;
	p->address = span;
	rc = uk_vma_map_anon(p->vas, &p->address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	if (rc) {
		uk_vas_put(p->vas);
		return NULL;
	}
	return space;
free_space:
	uk_free(kernel->a, space);
	return NULL;
}

static void vas_ref_gc(struct uk_thread *thread __unused, void *arg)
{
	struct vas_ref_probe *p = arg;

	__atomic_store_n(&p->gc_seen, true, __ATOMIC_RELEASE);
	uk_semaphore_up(&p->reaped);
}

static void vas_ref_dtor(struct uk_thread *thread)
{
	struct vas_ref_probe *p = thread->priv;

	p->dtor_vas_live = !__atomic_load_n(&p->releases, __ATOMIC_ACQUIRE);
	p->dtor_rebind_rc = uk_thread_set_vas(thread, NULL);
	__atomic_store_n(&p->dtor_seen, true, __ATOMIC_RELEASE);
}

static void vas_ref_term(struct uk_thread *thread)
{
	struct vas_ref_probe *p;

	if (thread->name != vas_ref_worker_name)
		return;
	p = thread->priv;
	p->term_vas_live = !__atomic_load_n(&p->releases, __ATOMIC_ACQUIRE);
	p->term_rebind_rc = uk_thread_set_vas(thread, NULL);
	__atomic_store_n(&p->term_seen, true, __ATOMIC_RELEASE);
}

UK_THREAD_INIT_PRIO_FLAGS(0, vas_ref_term, UK_PRIO_LATEST, 0);

static void vas_ref_worker(void *arg) __noreturn;

static void vas_ref_worker(void *arg)
{
	struct vas_ref_probe *p = arg;

	uk_semaphore_down(&p->start);
	p->active = uk_vas_get_active() == p->vas &&
		uk_paging_pt_get_active() == p->vas->pt;
	*(__u64 *)p->address = VAS_REF_VALUE;
	p->value = *(__u64 *)p->address;
	uk_sched_thread_exit2(vas_ref_gc, p);
}

UK_TESTCASE(vas_refs, current_binding)
{
	struct vas_ref_probe probe = { 0 };
	struct uk_thread *current = uk_thread_current();
	struct uk_vas *previous = current->vas;
	struct uk_vas *kernel = uk_vas_get_active();
	struct vas_ref_space *space;
	int rc;

	UK_TEST_EXPECT(kernel == uk_vas_get_kernel() &&
		       (!previous || previous == kernel));
	if (kernel != uk_vas_get_kernel() || (previous && previous != kernel))
		return;
	space = vas_ref_create(&probe);
	UK_TEST_EXPECT_NOT_NULL(space);
	if (!space)
		return;
	rc = uk_thread_set_vas(current, probe.vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_vas_put(probe.vas);
		return;
	}
	UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), probe.vas);
	uk_vas_put(probe.vas);
	UK_TEST_EXPECT_ZERO(probe.releases);
	*(__u64 *)probe.address = VAS_REF_VALUE;
	UK_TEST_EXPECT_SNUM_EQ(*(__u64 *)probe.address, VAS_REF_VALUE);
	rc = uk_thread_set_vas(current, previous);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		UK_CRASH("Cannot restore the kernel address space");
	UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), kernel);
	UK_TEST_EXPECT_SNUM_EQ(probe.releases, 1);
	UK_TEST_EXPECT_ZERO(probe.cleanup_rc);
	UK_TEST_EXPECT(probe.released_inactive);
}

UK_TESTCASE(vas_refs, thread_lifetime)
{
	struct vas_ref_probe probe = { 0 };
	struct uk_sched *local = uk_sched_current();
	struct uk_sched *target = local;
	struct vas_ref_space *space;
	struct uk_thread *thread;
	unsigned int cpus = uk_sched_started_count();
	unsigned int i;
	unsigned long irqf;
	__nsec elapsed;
	int rc;

	for (i = 0; i < cpus && cpus > 1 && target == local; i++)
		target = uk_sched_select_round_robin(local);
	UK_TEST_EXPECT(cpus == 1 || target != local);
	space = vas_ref_create(&probe);
	UK_TEST_EXPECT_NOT_NULL(space);
	if (!space)
		return;
	thread = uk_thread_create_fn1(local->a, vas_ref_worker, &probe,
				      local->a_stack, 0, local->a_auxstack, 0,
				      local->a_uktls, false,
				      vas_ref_worker_name, &probe,
				      vas_ref_dtor);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread) {
		uk_vas_put(probe.vas);
		return;
	}
	rc = uk_thread_set_vas(thread, probe.vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_thread_release(thread);
		uk_vas_put(probe.vas);
		return;
	}
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&space->vas.refs), 2);
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(thread, probe.vas));
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&space->vas.refs), 2);
	irqf = uk_lcpu_save_irqf();
	rc = uk_thread_set_vas(thread, NULL);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EWOULDBLOCK);
	UK_TEST_EXPECT_PTR_EQ(thread->vas, probe.vas);
	uk_vas_put(probe.vas);
	/* The unpublished thread alone must retain the VAS at this point. */
	UK_TEST_EXPECT_ZERO(probe.releases);
	rc = uk_sched_thread_add(target, thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_thread_release(thread);
		return;
	}
	UK_TEST_EXPECT_SNUM_EQ(uk_thread_set_vas(thread, NULL), -EBUSY);
	uk_semaphore_up(&probe.start);
	UK_TEST_EXPECT(uk_semaphore_down_to(&probe.reaped, VAS_REF_TIMEOUT)
		       < VAS_REF_TIMEOUT / 2);
	elapsed = uk_semaphore_down_to(&probe.released, VAS_REF_TIMEOUT);
	UK_TEST_EXPECT(elapsed < VAS_REF_TIMEOUT / 2);
	if (elapsed >= VAS_REF_TIMEOUT)
		UK_CRASH("Owned VAS worker did not finish");
	UK_TEST_EXPECT_SNUM_EQ(probe.term_rebind_rc, -ESRCH);
	UK_TEST_EXPECT_SNUM_EQ(probe.dtor_rebind_rc, -ESRCH);
	UK_TEST_EXPECT(probe.active);
	UK_TEST_EXPECT_SNUM_EQ(probe.value, VAS_REF_VALUE);
	UK_TEST_EXPECT_SNUM_EQ(probe.releases, 1);
	UK_TEST_EXPECT_ZERO(probe.cleanup_rc);
	UK_TEST_EXPECT(probe.released_after_gc);
	UK_TEST_EXPECT(probe.released_after_term);
	UK_TEST_EXPECT(probe.released_after_dtor);
	UK_TEST_EXPECT(probe.released_inactive);
	UK_TEST_EXPECT(probe.term_vas_live);
	UK_TEST_EXPECT(probe.dtor_vas_live);
}

_UK_TESTSUITE(vas_refs, NULL);

static void __constructor run_vas_refs(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(vas_refs));
}
