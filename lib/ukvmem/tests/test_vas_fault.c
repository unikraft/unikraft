/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/paging.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/vmem.h>
#include <uk/vmem/vma_ops.h>

#if CONFIG_ARCH_X86_64 && CONFIG_LIBUKPLAT_NATIVE_EXCEPT
struct nested_fault_probe {
	struct uk_paging_deferred_call call;
	__vaddr_t kernel_address;
	__vaddr_t private_address;
	unsigned int calls;
	__u64 first;
	__u64 second;
	bool done;
	bool irq_completion;
};

static struct nested_fault_probe nested_probe;

static void nested_fault_release(void *arg)
{
	struct nested_fault_probe *probe = arg;
	__u64 *value = (__u64 *)(probe->kernel_address +
				probe->calls * UK_PAGING_PAGE_SIZE);
	unsigned long irqf;

	probe->irq_completion &= !uk_lcpu_irqs_disabled();
	irqf = uk_lcpu_save_irqf();

	/* We are still on the outer page-fault stack. The inner fault must
	 * preserve that stack, including when the nesting mode is recursive.
	 */
	uk_lcpu_except_push_nested();
	uk_lcpu_except_push_nested();
	__atomic_store_n(value, 3, __ATOMIC_RELAXED);
	uk_lcpu_except_pop_nested();
	uk_lcpu_except_pop_nested();
	uk_lcpu_restore_irqf(irqf);
	probe->calls++;
}

static int nested_anon_fault(struct uk_vma *vma, struct uk_vm_fault *fault)
{
	struct uk_paging_free_batch *batch = vma->vas->pt->defer_batch;
	int rc;

	if (fault->type & UK_VMA_FAULT_NONPRESENT) {
		rc = uk_vma_anon_ops.fault(vma, fault);
		if (rc)
			return rc;
	} else {
		/* Exercise restoring write permission on an existing private
		 * frame, as with a COW page whose last owner remains.
		 */
		UK_ASSERT(fault->type & UK_VMA_FAULT_IRQS_ENABLED);
		UK_ASSERT((fault->type & UK_VMA_FAULT_ACCESSTYPE) ==
			  UK_VMA_FAULT_WRITE);
	}
	UK_ASSERT(batch);
	uk_paging_batch_defer(batch, &nested_probe.call,
			      nested_fault_release, &nested_probe);
	return 0;
}

static const struct uk_vma_ops nested_anon_ops = {
	.fault = nested_anon_fault,
	.unmap = uk_vma_op_unmap,
	.set_attr = uk_vma_op_set_attr,
	.advise = uk_vma_op_advise,
};

static void nested_fault_thread(void *arg) __noreturn;

static void nested_fault_thread(void *arg)
{
	struct nested_fault_probe *probe = arg;
	__u64 *first = (__u64 *)probe->private_address;
	__u64 *second = (__u64 *)(probe->private_address + UK_PAGING_PAGE_SIZE);

	__atomic_store_n(first, 7, __ATOMIC_RELAXED);
	__atomic_store_n(second, 9, __ATOMIC_RELAXED);
	probe->first = __atomic_load_n(first, __ATOMIC_RELAXED);
	probe->second = __atomic_load_n(second, __ATOMIC_RELAXED);
	__atomic_store_n(&probe->done, true, __ATOMIC_RELEASE);
	uk_thread_exit();
}

UK_TESTCASE(vas_fault, complete_with_nested_page_fault)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_sched *sched = uk_sched_current();
	const __sz shared_len = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	const unsigned long read_only = UK_PAGING_PAGE_ATTR_PROT_READ;
	struct uk_pagetable child_pt;
	struct uk_vas child;
	struct uk_thread *thread;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__vaddr_t private_address = shared_len;
	__vaddr_t kernel_page;
	__u64 *kernel_next;
	__u64 kernel_first, kernel_second;
	int rc;

	rc = uk_vma_map_anon(kernel, &address, 4 * UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_paging_pt_clone_shared(&child_pt, kernel->pt, 0, shared_len);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto unmap_kernel;
	rc = uk_vas_init(&child, &child_pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	rc = uk_vma_map(&child, &private_address, 2 * UK_PAGING_PAGE_SIZE,
			UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL, &nested_anon_ops,
			NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_child;
	for (unsigned int round = 0; round < 2; round++) {
		if (round) {
			rc = uk_paging_page_set_attr(&child_pt, private_address,
						     2, read_only, 0);
			UK_TEST_EXPECT_ZERO(rc);
			if (rc)
				goto destroy_child;
		}
		kernel_page = address + round * 2 * UK_PAGING_PAGE_SIZE;
		nested_probe = (struct nested_fault_probe){
			.kernel_address = kernel_page,
			.private_address = private_address,
			.irq_completion = true,
		};
		thread = uk_thread_create_fn1(sched->a,
					      nested_fault_thread,
					      &nested_probe,
					      sched->a_stack, 0,
					      sched->a_auxstack, 0,
					      sched->a_uktls, false,
					      "nested-fault",
					      NULL, NULL);
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			goto destroy_child;
		thread->vas = &child;
		rc = uk_sched_thread_add(sched, thread);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc) {
			uk_thread_release(thread);
			goto destroy_child;
		}
		for (unsigned int i = 0;
		     !__atomic_load_n(&nested_probe.done, __ATOMIC_ACQUIRE) &&
		     i < 64; ++i)
			uk_sched_yield();
		if (!__atomic_load_n(&nested_probe.done, __ATOMIC_ACQUIRE))
			UK_CRASH("Nested page-fault test did not terminate\n");
		UK_TEST_EXPECT_SNUM_EQ(nested_probe.calls, 2);
		UK_TEST_EXPECT(nested_probe.irq_completion);
		UK_TEST_EXPECT_SNUM_EQ(nested_probe.first, 7);
		UK_TEST_EXPECT_SNUM_EQ(nested_probe.second, 9);
		kernel_next = (__u64 *)nested_probe.kernel_address;
		kernel_first = __atomic_load_n(kernel_next, __ATOMIC_RELAXED);
		kernel_next += UK_PAGING_PAGE_SIZE / sizeof(*kernel_next);
		kernel_second = __atomic_load_n(kernel_next, __ATOMIC_RELAXED);
		UK_TEST_EXPECT_SNUM_EQ(kernel_first, 3);
		UK_TEST_EXPECT_SNUM_EQ(kernel_second, 3);
	}

destroy_child:
	uk_vas_destroy(&child);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&child_pt,
					      UK_PAGING_PAGE_FLAG_KEEP_FRAMES));
unmap_kernel:
	rc = uk_vma_unmap(kernel, address, 4 * UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
}

_UK_TESTSUITE(vas_fault, NULL);

static void __constructor run_vas_fault(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(vas_fault));
}
#endif
