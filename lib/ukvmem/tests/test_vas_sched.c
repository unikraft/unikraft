/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/falloc.h>
#include <uk/paging.h>
#include <uk/plat/time.h>
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
	struct uk_paging_free_batch *current;
	struct uk_pagetable unrelated = { 0 };
	struct deferred_release_probe probe = { .pt = kernel->pt };
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	unsigned long irqf;
	int rc;

	rc = uk_vma_map_anon(kernel, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	probe.address = address;

	irqf = uk_lcpu_save_irqf();
	UK_TEST_EXPECT_NULL(uk_paging_batch_current(kernel->pt));
	uk_paging_batch_begin(&batch, kernel->pt);
	UK_TEST_EXPECT_NULL(uk_paging_batch_current(&unrelated));
	uk_paging_batch_defer(&batch, &calls[0], deferred_release, &probe);
	rc = uk_paging_page_set_attr(kernel->pt, address, 1,
				     UK_PAGING_PAGE_ATTR_PROT_READ, 0);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_ZERO(probe.calls);
	/* The inner operation has dropped its own binding, but the VAS batch
	 * must still accept retirement of backend-owned backing afterwards.
	 */
	current = uk_paging_batch_current(kernel->pt);
	UK_TEST_EXPECT_PTR_EQ(current, &batch);
	if (current)
		uk_paging_batch_defer(current, &calls[1],
				      deferred_release, &probe);
	uk_paging_batch_end(&batch);
	UK_TEST_EXPECT_NULL(uk_paging_batch_current(kernel->pt));
	uk_lcpu_restore_irqf(irqf);
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
	__vaddr_t kernel_address;
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
	/* This page is first populated while the child page table is active. */
	__atomic_store_n((__u64 *)probe->kernel_address, 3, __ATOMIC_RELAXED);
	uk_sched_yield();
	probe->restored = uk_vas_get_active() == probe->vas &&
			  __atomic_load_n(value, __ATOMIC_RELAXED) == 2;
	__atomic_store_n(&probe->done, true, __ATOMIC_RELEASE);
	uk_thread_exit();
}

static int vas_switch_bind_current(struct uk_vas *vas)
{
#if CONFIG_LIBUKVMEM_VAS_REFCOUNT
	return uk_thread_set_vas(uk_thread_current(), vas);
#else
	int rc = uk_vas_set_active(vas ? vas : uk_vas_get_kernel());

	if (!rc)
		uk_thread_current()->vas = vas;
	return rc;
#endif
}

UK_TESTCASE(vas_sched, switch_distinct_address_spaces)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_vas *saved_binding = uk_thread_current()->vas;
	struct uk_sched *sched = uk_sched_current();
	struct uk_pagetable root_pt, child_pt;
	struct uk_vas root, child;
	struct vas_switch_probe probe = { .vas = &child };
	struct uk_thread *thread = NULL;
	const __sz shared_len = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	__vaddr_t address = shared_len;
	__vaddr_t kernel_address = UK_PAGING_VADDR_ANY;
	__u64 root_value, shared_value;
	int rc;

	UK_TEST_EXPECT(kernel && uk_vas_get_active() == kernel);
	if (!kernel || uk_vas_get_active() != kernel)
		return;

	/* Neither private mapping belongs in the shared kernel VAS. */
	rc = uk_paging_pt_clone_shared(&root_pt, kernel->pt, 0, shared_len);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_vas_init(&root, &root_pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_root_pt;
	rc = uk_vma_map_anon(&root, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_root;

	rc = uk_vma_map_anon(kernel, &kernel_address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_root;
	UK_TEST_EXPECT(kernel_address < shared_len);

	rc = uk_paging_pt_clone_shared(&child_pt, kernel->pt, 0, shared_len);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto unmap_shared;
	UK_TEST_EXPECT(uk_paging_pt_get_owner(&child_pt, kernel_address) ==
		       kernel->pt);
	UK_TEST_EXPECT(uk_paging_pt_get_owner(&child_pt, address) == &child_pt);
	UK_TEST_EXPECT_SNUM_EQ(uk_paging_pt_free(kernel->pt, 0), -EBUSY);
	rc = uk_paging_page_set_attr(&child_pt, kernel_address, 1, 0, 0);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EACCES);
	rc = uk_vas_init(&child, &child_pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;

	probe.address = address;
	probe.kernel_address = kernel_address;
	rc = uk_vma_map_anon(&child, &probe.address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_child;
	UK_TEST_EXPECT(probe.address == address);
	rc = vas_switch_bind_current(&root);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_child;
	__atomic_store_n((__u64 *)address, 1, __ATOMIC_RELAXED);

	/* Allocate the kernel stack and TLS after cloning the page table. */
	thread = uk_thread_create_fn1(sched->a, vas_switch_thread, &probe,
				      sched->a_stack, 0, sched->a_auxstack, 0,
				      sched->a_uktls, false, "vas-switch",
				      NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto destroy_child;

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
	UK_TEST_EXPECT(uk_vas_get_active() == &root);
	root_value = __atomic_load_n((__u64 *)address, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_SNUM_EQ(root_value, 1);
	shared_value = __atomic_load_n((__u64 *)kernel_address,
				       __ATOMIC_RELAXED);
	UK_TEST_EXPECT_SNUM_EQ(shared_value, 3);

destroy_child:
	UK_TEST_EXPECT_ZERO(vas_switch_bind_current(saved_binding));
	uk_vas_destroy(&child);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&child_pt,
					      UK_PAGING_PAGE_FLAG_KEEP_FRAMES));
	if (thread)
		uk_thread_release(thread);
unmap_shared:
	rc = uk_vma_unmap(kernel, kernel_address, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
destroy_root:
	uk_vas_destroy(&root);
free_root_pt:
	rc = uk_paging_pt_free(&root_pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	UK_TEST_EXPECT_ZERO(rc);
}

struct shared_tlb_probe {
	__vaddr_t address;
	bool ready;
	bool replaced;
	bool reaped;
	__u64 initial;
	__u64 observed;
};

static bool wait_for_flag(bool *flag)
{
	const __nsec deadline = ukplat_monotonic_clock() +
				5 * UKARCH_NSEC_PER_SEC;

	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (ukplat_monotonic_clock() >= deadline)
			return false;
		uk_arch_spinwait();
	}
	return true;
}

static void shared_tlb_thread(void *arg) __noreturn;

static void shared_tlb_thread(void *arg)
{
	struct shared_tlb_probe *probe = arg;
	__u64 *value = (__u64 *)probe->address;

	probe->initial = __atomic_load_n(value, __ATOMIC_RELAXED);
	__atomic_store_n(&probe->ready, true, __ATOMIC_RELEASE);
	if (wait_for_flag(&probe->replaced))
		probe->observed = __atomic_load_n(value, __ATOMIC_RELAXED);
	uk_thread_exit();
}

static void shared_tlb_reaped(struct uk_thread *thread)
{
	struct shared_tlb_probe *probe = thread->priv;

	/* The remote scheduler has left the child's stack and address space. */
	__atomic_store_n(&probe->reaped, true, __ATOMIC_RELEASE);
}

static int replace_shared_page(struct uk_pagetable *pt __unused,
			       __vaddr_t address __unused,
			       __vaddr_t pt_address __unused,
			       unsigned int level __maybe_unused,
			       __pte_t *pte, void *arg)
{
	__paddr_t *paddr = arg;

	*pte = UK_PAGING_PT_Lx_PTE_SET_PADDR(*pte, level, *paddr);
	return 0;
}

#if CONFIG_LIBUKPAGING_STATS
UK_TESTCASE(vas_sched, mapped_leaf_count_on_replacement)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz shared_len = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	const unsigned int level = UK_PAGING_PAGE_LEVEL;
	const unsigned long flags = UK_PAGING_PAGE_FLAG_FORCE_SIZE |
		UK_PAGING_PAGE_FLAG_SIZE(UK_PAGING_PAGE_LEVEL);
	struct uk_pagetable pt;
	__paddr_t frame = UK_PAGING_PADDR_ANY;
	struct uk_paging_page_mapx mapx = {
		.map = replace_shared_page,
		.ctx = &frame,
	};
	unsigned long initial, mapped;
	unsigned int i;
	int rc;

	/* This page table is never activated or published to another CPU, so
	 * unrelated kernel faults cannot change the sampled leaf counts.
	 */
	rc = uk_paging_pt_clone_shared(&pt, kernel->pt, 0, shared_len);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	initial = pt.nr_lx_pages[level];
	rc = pt.fa->falloc(pt.fa, &frame, 1, 0);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	rc = uk_paging_page_map(&pt, shared_len, frame, 1,
				UK_PAGING_PAGE_ATTR_PROT_RW, flags);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_frame;
	mapped = initial + 1;
	UK_TEST_EXPECT_SNUM_EQ(pt.nr_lx_pages[level], mapped);
	for (i = 0; i < 64; i++) {
		rc = uk_paging_page_mapx(&pt, shared_len, frame, 1,
					 UK_PAGING_PAGE_ATTR_PROT_RW,
					 flags, &mapx);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(pt.nr_lx_pages[level], mapped);
		if (rc)
			break;
	}
	rc = uk_paging_page_unmap(&pt, shared_len, 1,
				  UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(pt.nr_lx_pages[level], initial);

free_frame:
	/* Keep ownership of the explicitly allocated frame on error paths. */
	rc = uk_paging_pt_free(&pt, UK_PAGING_PAGE_FLAG_KEEP_FRAMES);
	UK_TEST_EXPECT_ZERO(rc);
	if (!rc)
		uk_ffree(kernel->pt->fa, frame, 1);
	return;
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&pt, 0));
}
#endif /* CONFIG_LIBUKPAGING_STATS */

UK_TESTCASE(vas_sched, invalidate_shared_kernel_tlb)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_sched *local = uk_sched_current();
	struct uk_sched *remote = local;
	const unsigned int cpus = uk_sched_started_count();
	const __sz shared_len = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct uk_pagetable child_pt;
	struct uk_vas child;
	struct shared_tlb_probe probe = { 0 };
	struct uk_thread *thread;
	__paddr_t new_frame = UK_PAGING_PADDR_ANY;
	__paddr_t old_frame;
	struct uk_paging_page_mapx mapx = {
		.map = replace_shared_page,
		.ctx = &new_frame,
	};
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__vaddr_t temporary;
	bool replaced;
	int rc;

	if (cpus < 2)
		return;
	for (unsigned int i = 0; i < cpus && remote == local; ++i)
		remote = uk_sched_select_round_robin(local);
	UK_TEST_EXPECT(remote != local);
	if (remote == local)
		return;

	rc = uk_vma_map_anon(kernel, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	__atomic_store_n((__u64 *)address, 1, __ATOMIC_RELAXED);
	old_frame = uk_paging_virt_to_phys(address);
	rc = kernel->pt->fa->falloc(kernel->pt->fa, &new_frame, 1, 0);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto unmap_kernel;
	UK_TEST_EXPECT(new_frame != old_frame);
	temporary = uk_paging_page_kmap(kernel->pt, new_frame, 1, 0);
	UK_TEST_EXPECT(temporary != UK_PAGING_VADDR_INV);
	if (temporary == UK_PAGING_VADDR_INV)
		goto free_frame;
	__atomic_store_n((__u64 *)temporary, 2, __ATOMIC_RELAXED);
	uk_paging_page_kunmap(kernel->pt, temporary, 1, 0);

	rc = uk_paging_pt_clone_shared(&child_pt, kernel->pt, 0, shared_len);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_frame;
	rc = uk_vas_init(&child, &child_pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	probe.address = address;
	thread = uk_thread_create_fn1(remote->a, shared_tlb_thread, &probe,
				      remote->a_stack, 0, remote->a_auxstack, 0,
				      remote->a_uktls, false, "shared-tlb",
				      &probe, shared_tlb_reaped);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto destroy_child;
	thread->vas = &child;
	rc = uk_sched_thread_add(remote, thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_thread_release(thread);
		goto destroy_child;
	}
	if (!wait_for_flag(&probe.ready))
		UK_CRASH("Shared TLB test thread did not start\n");
	UK_TEST_EXPECT_SNUM_EQ(probe.initial, 1);

	/* The remote CPU retains the old translation while spinning in the
	 * child VAS. Keep its old frame allocated until after the second read,
	 * so reuse of that frame cannot hide a missing TLB invalidation.
	 */
	rc = uk_paging_page_mapx(kernel->pt, address, 0, 1,
				 UK_PAGING_PAGE_ATTR_PROT_RW, 0, &mapx);
	UK_TEST_EXPECT_ZERO(rc);
	replaced = !rc;
	__atomic_store_n(&probe.replaced, true, __ATOMIC_RELEASE);
	if (!wait_for_flag(&probe.reaped))
		UK_CRASH("Shared TLB test thread did not leave its VAS\n");
	UK_TEST_EXPECT_SNUM_EQ(probe.observed, 2);
	if (replaced) {
		rc = kernel->pt->fa->ffree(kernel->pt->fa, old_frame, 1);
		UK_TEST_EXPECT_ZERO(rc);
		/* The target VMA owns the replacement frame now. */
		new_frame = UK_PAGING_PADDR_INV;
	}

destroy_child:
	uk_vas_destroy(&child);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&child_pt,
					      UK_PAGING_PAGE_FLAG_KEEP_FRAMES));
	UK_TEST_EXPECT(!child_pt.shared_parent);
free_frame:
	if (new_frame != UK_PAGING_PADDR_INV) {
		rc = kernel->pt->fa->ffree(kernel->pt->fa, new_frame, 1);
		UK_TEST_EXPECT_ZERO(rc);
	}
unmap_kernel:
	rc = uk_vma_unmap(kernel, address, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
}

_UK_TESTSUITE(vas_sched, NULL);

static void __constructor run_vas_sched(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(vas_sched));
}
