/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/plat/spinlock.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem.h>
#include "../vmem.h"

#define TRANSITION_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)
#define TRANSITION_BEFORE 0x11223344U
#define TRANSITION_AFTER  0x55667788U

struct blocked_vma {
	struct uk_vma base;
	unsigned int *destroyed;
};

static int blocked_vma_new(struct uk_vas *vas __unused,
			   __vaddr_t address __unused, __sz len __unused,
			   void *arg, unsigned long attr __unused,
			   unsigned long *flags __unused, struct uk_vma **vma)
{
	struct blocked_vma *blocked = arg;

	*vma = &blocked->base;
	return 0;
}

static int blocked_vma_split(struct uk_vma *vma __unused,
			     __vaddr_t address __unused,
			     struct uk_vma **new_vma __unused)
{
	return -EOPNOTSUPP;
}

static void blocked_vma_destroy(struct uk_vma *vma)
{
	struct blocked_vma *blocked = __containerof(vma, struct blocked_vma,
						   base);

	(*blocked->destroyed)++;
}

static const struct uk_vma_ops blocked_ops = {
	.new = blocked_vma_new,
	.split = blocked_vma_split,
	.destroy = blocked_vma_destroy,
};

UK_TESTCASE(vas_transitions, masked_mutations_leave_mappings_intact)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct vmem_vas_transition transition = { 0 }, other = { 0 };
	struct uk_pagetable pt;
	struct uk_vas vas;
	struct blocked_vma *blocked;
	unsigned int destroyed = 0;
	unsigned int level = UK_PAGING_PAGE_LEVEL;
	__pte_t before, after;
	__vaddr_t address = span, rejected = span + UK_PAGING_PAGE_SIZE;
	unsigned long irqf;
	int results[8];
	int rc;

	rc = uk_paging_pt_clone_shared(&pt, kernel->pt, 0, span);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	rc = uk_vas_init(&vas, &pt, kernel->a);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_pt;
	rc = uk_vma_map_anon(&vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_vas;
	rc = uk_paging_pt_walk(&pt, address, &level, NULL, &before);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto destroy_vas;
	blocked = uk_malloc(kernel->a, sizeof(*blocked));
	UK_TEST_EXPECT_NOT_NULL(blocked);
	if (!blocked)
		goto destroy_vas;
	blocked->base.name = NULL;
	blocked->destroyed = &destroyed;
	rc = vmem_vas_transition_begin(&vas, &transition);
	if (rc) {
		UK_TEST_EXPECT_ZERO(rc);
		uk_free(kernel->a, blocked);
		goto destroy_vas;
	}
	irqf = uk_lcpu_save_irqf();
	results[0] = uk_vma_unmap(&vas, address, UK_PAGING_PAGE_SIZE, 0);
	results[1] = uk_vma_unmap_at(&vas, address);
	results[2] = uk_vma_set_attr(&vas, address, UK_PAGING_PAGE_SIZE,
				     UK_PAGING_PAGE_ATTR_PROT_READ, 0);
	results[3] = uk_vma_advise(&vas, address, UK_PAGING_PAGE_SIZE,
				   UK_VMA_ADV_DONTNEED, 0);
	results[4] = uk_vma_sync(&vas, address, UK_PAGING_PAGE_SIZE);
	results[5] = uk_vma_map(&vas, &rejected, UK_PAGING_PAGE_SIZE,
				UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL,
				&blocked_ops, blocked);
	results[6] = vmem_vas_transition_begin(&vas, &other);
	results[7] = uk_vas_store_u32(&vas, address, TRANSITION_AFTER);
	uk_lcpu_restore_irqf(irqf);
	vmem_vas_transition_end(&transition);
	UK_TEST_EXPECT_ZERO(rc);
	for (unsigned int i = 0; i < ARRAY_SIZE(results); i++)
		UK_TEST_EXPECT_SNUM_EQ(results[i], -EWOULDBLOCK);
	UK_TEST_EXPECT_NULL(transition.vas);
	UK_TEST_EXPECT_NULL(other.vas);
	UK_TEST_EXPECT_SNUM_EQ(destroyed, 1);
	UK_TEST_EXPECT_NOT_NULL(uk_vma_find(&vas, address));
	UK_TEST_EXPECT_NULL(uk_vma_find(&vas, rejected));
	level = UK_PAGING_PAGE_LEVEL;
	rc = uk_paging_pt_walk(&pt, address, &level, NULL, &after);
	UK_TEST_EXPECT_ZERO(rc);
	if (!rc)
		UK_TEST_EXPECT_SNUM_EQ(after, before);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(&vas, address, TRANSITION_BEFORE));
	UK_TEST_EXPECT_SNUM_EQ(vmem_vas_transition_begin(kernel, &other),
			       -EINVAL);
destroy_vas:
	uk_vas_destroy(&vas);
free_pt:
	UK_TEST_EXPECT_ZERO(uk_paging_pt_free(&pt, 0));
}

#if CONFIG_ARCH_X86_64 && CONFIG_LIBUKPLAT_NATIVE_EXCEPT
#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/event.h>
#include <uk/pcpuvar.h>

struct transition_probe {
	struct uk_semaphore reaped;
	__vaddr_t address;
	__u32 *physical;
	unsigned int cpu;
	__u32 ipi_value;
	__u32 final_value;
	int cleanup_rc;
	bool ipi_completed;
	bool start;
	bool fault_seen;
	bool fault_masked;
	bool fault_irqs_enabled;
	bool ipi_seen;
	bool completed;
};

static struct transition_probe *transition_probe;

static int transition_fault_observe(void *arg)
{
	__u64 orig_gsbase = uk_arch_x86_64_enter_kernel_gsbase();
	struct uk_lcpu_except_err_ctx *ctx = arg;
	struct transition_probe *p = __atomic_load_n(&transition_probe,
						     __ATOMIC_ACQUIRE);

	if (p && uk_lcpu_except_err_ctx_get_fault_addr(ctx) == p->address &&
	    uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx) == p->cpu) {
		p->fault_masked = uk_lcpu_irqs_disabled();
		p->fault_irqs_enabled = uk_lcpu_regs_get((struct uk_lcpu_regs *)
			uk_lcpu_except_err_ctx_get_regs(ctx), RFLAGS) &
			UK_ARCH_X86_64_RFLAGS_IF;
		__atomic_store_n(&p->fault_seen, true, __ATOMIC_RELEASE);
	}
	uk_arch_x86_64_exit_kernel_gsbase(orig_gsbase);
	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT,
		      transition_fault_observe,
		      UK_PRIO_BEFORE(CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO));

static void transition_ipi(struct uk_lcpu_regs *regs __unused, void *arg)
{
	struct transition_probe *p = arg;

	p->ipi_value = __atomic_load_n(p->physical, __ATOMIC_RELAXED);
	p->ipi_completed = __atomic_load_n(&p->completed, __ATOMIC_ACQUIRE);
	__atomic_store_n(&p->ipi_seen, true, __ATOMIC_RELEASE);
}

static void transition_worker(void *arg) __noreturn;

static void transition_worker(void *arg)
{
	struct transition_probe *p = arg;

	while (!__atomic_load_n(&p->start, __ATOMIC_ACQUIRE))
		uk_arch_spinwait();
	__atomic_store_n((__u32 *)p->address, TRANSITION_AFTER,
			 __ATOMIC_RELAXED);
	__atomic_store_n(&p->completed, true, __ATOMIC_RELEASE);
	uk_thread_exit();
}

static void transition_worker_reaped(struct uk_thread *thread)
{
	struct transition_probe *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

static void transition_wait_flag(bool *flag)
{
	__nsec end = ukplat_monotonic_clock() + TRANSITION_TIMEOUT;

	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (ukplat_monotonic_clock() >= end)
			UK_CRASH("VAS transition test timed out\n");
		uk_arch_spinwait();
	}
}

static void transition_remote_fault(struct uk_sched *sched,
				    struct transition_probe *probe)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	const __sz span = UK_PAL_PAGE_Lx_SIZE(UK_PAL_PT_LEVELS - 1);
	struct vmem_vas_transition transition = { 0 };
	struct uk_lcpu_func fn = { .fn = transition_ipi, .user = probe };
	struct uk_pagetable pt;
	struct uk_vas vas;
	struct uk_thread *thread;
	__vaddr_t address = span, mapped;
	__nsec end;
	unsigned int level = UK_PAGING_PAGE_LEVEL, count = 1;
	__u64 cpu = sched->lcpu_idx;
	__pte_t pte;
	int rc;

	uk_semaphore_init(&probe->reaped, 0);
	rc = uk_paging_pt_clone_shared(&pt, kernel->pt, 0, span);
	if (rc || uk_vas_init(&vas, &pt, kernel->a))
		UK_CRASH("Cannot create VAS transition test address space\n");
	rc = uk_vma_map_anon(&vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, UK_VMA_MAP_POPULATE,
			     NULL);
	if (rc || uk_vas_store_u32(&vas, address, TRANSITION_BEFORE))
		UK_CRASH("Cannot prepare VAS transition test page\n");
	rc = uk_paging_pt_walk(&pt, address, &level, NULL, &pte);
	if (rc || !UK_PAGING_PT_Lx_PTE_PRESENT(pte, level))
		UK_CRASH("Missing VAS transition test page\n");
	mapped = uk_paging_page_kmap(&pt, UK_PAGING_PT_Lx_PTE_PADDR(pte, level),
				     1, 0);
	if (mapped == UK_PAGING_VADDR_INV)
		UK_CRASH("Cannot inspect VAS transition test page\n");
	probe->physical = (__u32 *)mapped;
	probe->address = address;
	rc = uk_paging_page_set_attr(&pt, address, 1,
				     UK_PAGING_PAGE_ATTR_PROT_READ, 0);
	if (rc)
		UK_CRASH("Cannot protect VAS transition test page\n");
	thread = uk_thread_create_fn1(sched->a, transition_worker, probe,
				      sched->a_stack, 0, sched->a_auxstack, 0,
				      sched->a_uktls, false, "vas-transition",
				      probe, transition_worker_reaped);
	if (!thread)
		UK_CRASH("Cannot create VAS transition test worker\n");
	/* Unpublished thread and borrowed VAS; no owned-VAS API is required. */
	thread->vas = &vas;
	__atomic_store_n(&transition_probe, probe, __ATOMIC_RELEASE);
	if (uk_sched_thread_add(sched, thread))
		UK_CRASH("Cannot publish VAS transition test worker\n");
	if (vmem_vas_transition_begin(&vas, &transition))
		UK_CRASH("Cannot start VAS transition test\n");
	__atomic_store_n(&probe->start, true, __ATOMIC_RELEASE);
	transition_wait_flag(&probe->fault_seen);
	end = ukplat_monotonic_clock() + TRANSITION_TIMEOUT;
	do {
		count = 1;
		rc = uk_lcpu_run(&cpu, &count, &fn, UK_LCPU_RFLG_DONOTBLOCK);
		if (rc == -EAGAIN && ukplat_monotonic_clock() >= end)
			UK_CRASH("VAS transition test IPI queue timed out\n");
		uk_arch_spinwait();
	} while (rc == -EAGAIN);
	if (rc || count != 1)
		UK_CRASH("Cannot queue VAS transition test IPI\n");
	transition_wait_flag(&probe->ipi_seen);
	/* This batch itself needs the CPU currently waiting in its fault. */
	rc = uk_paging_page_set_attr(&pt, address, 1,
				     UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	if (rc)
		UK_CRASH("Cannot complete VAS transition test mapping\n");
	vmem_vas_transition_end(&transition);
	if (uk_semaphore_down_to(&probe->reaped, TRANSITION_TIMEOUT) ==
	    __NSEC_MAX)
		UK_CRASH("VAS transition test worker was not reaped\n");
	__atomic_store_n(&transition_probe, NULL, __ATOMIC_RELEASE);
	probe->final_value = __atomic_load_n(probe->physical, __ATOMIC_RELAXED);
	uk_paging_page_kunmap(&pt, mapped, 1, 0);
	uk_vas_destroy(&vas);
	probe->cleanup_rc = uk_paging_pt_free(&pt, 0);
}
#endif

UK_TESTCASE(vas_transitions, hardware_wait_accepts_completion_ipi)
{
#if CONFIG_ARCH_X86_64 && CONFIG_LIBUKPLAT_NATIVE_EXCEPT
	struct uk_sched *local = uk_sched_current();
	unsigned int cpus = uk_sched_started_count();
	unsigned int remote = 0;

	for (unsigned int i = 0; i < cpus; i++) {
		struct uk_sched *sched = uk_sched_select_round_robin(local);
		struct transition_probe probe = { .cpu = sched->lcpu_idx };

		if (sched->lcpu_idx == local->lcpu_idx)
			continue;
		transition_remote_fault(sched, &probe);
		UK_TEST_EXPECT(probe.fault_masked);
		UK_TEST_EXPECT(probe.fault_irqs_enabled);
		UK_TEST_EXPECT(!probe.ipi_completed);
		UK_TEST_EXPECT_SNUM_EQ(probe.ipi_value, TRANSITION_BEFORE);
		UK_TEST_EXPECT(probe.completed);
		UK_TEST_EXPECT_SNUM_EQ(probe.final_value, TRANSITION_AFTER);
		UK_TEST_EXPECT_ZERO(probe.cleanup_rc);
		remote++;
	}
	UK_TEST_EXPECT_SNUM_EQ(remote, cpus - 1);
	uk_pr_info("VAS_TRANSITION_PASS cpus=%u remote=%u\n", cpus, remote);
#endif
}

_UK_TESTSUITE(vas_transitions, NULL);

static void __constructor run_vas_transitions(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(vas_transitions));
}
