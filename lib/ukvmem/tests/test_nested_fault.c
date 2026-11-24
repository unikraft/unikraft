/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/crash.h>
#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/paging.h>
#include <uk/pcpuvar.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem.h>

struct nested_probe {
	__vaddr_t outer;
	__vaddr_t inner;
	unsigned int faults;
	bool active;
	bool reaped;
};

static struct nested_probe probes[CONFIG_UKPLAT_CPU_MAXCOUNT];
static unsigned long long nested_cpu_mask;
static struct uk_semaphore nested_done;

static int observe_outer_fault(void *arg)
{
	__u64 orig_gsbase = uk_arch_x86_64_enter_kernel_gsbase();
	struct uk_lcpu_except_err_ctx *ctx = arg;
	__vaddr_t address = uk_lcpu_except_err_ctx_get_fault_addr(ctx);
	unsigned int cpu = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);
	struct nested_probe *probe;
	__vaddr_t offset;

	if (cpu >= CONFIG_UKPLAT_CPU_MAXCOUNT)
		goto out;
	probe = &probes[cpu];
	if (!__atomic_load_n(&probe->active, __ATOMIC_ACQUIRE) ||
	    address < probe->outer ||
	    address - probe->outer >= 2 * UK_PAGING_PAGE_SIZE)
		goto out;
	offset = UK_PAGING_PAGE_ALIGN_DOWN(address - probe->outer);

	/* Run before the VMA handler acquires any paging lock. The inner
	 * fault must preserve this hardware exception frame and return here.
	 */
	uk_lcpu_except_push_nested();
	uk_lcpu_except_push_nested();
	__atomic_store_n((__u64 *)(probe->inner + offset), 3, __ATOMIC_RELAXED);
	uk_lcpu_except_pop_nested();
	uk_lcpu_except_pop_nested();
	probe->faults++;
out:
	uk_arch_x86_64_exit_kernel_gsbase(orig_gsbase);
	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT, observe_outer_fault,
		      UK_PRIO_BEFORE(CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO));

static void nested_fault_worker(void *arg) __noreturn;

static void nested_fault_worker(void *arg)
{
	struct nested_probe *probe = arg;
	unsigned int cpu = uk_pcpuvar_current_get(uk_pcpuvar_cpu_idx);

	__atomic_fetch_or(&nested_cpu_mask, 1ULL << cpu, __ATOMIC_RELAXED);
	__atomic_store_n((__u64 *)probe->outer, 7, __ATOMIC_RELAXED);
	__atomic_store_n((__u64 *)(probe->outer + UK_PAGING_PAGE_SIZE), 9,
			 __ATOMIC_RELAXED);
	uk_thread_exit();
}

static void nested_fault_reaped(struct uk_thread *thread)
{
	struct nested_probe *probe = thread->priv;

	__atomic_store_n(&probe->reaped, true, __ATOMIC_RELEASE);
	uk_semaphore_up(&nested_done);
}

UK_TESTCASE(native_nested, nested_hardware_page_faults)
{
	struct uk_vas *vas = uk_vas_get_active();
	struct uk_sched *local = uk_sched_current();
	unsigned int cpus = uk_sched_started_count();
	unsigned long long expected_mask = 0;
	unsigned long long mask;
	unsigned int faults = 0;
	const __nsec wait_ns = 5 * UKARCH_NSEC_PER_SEC;
	int rc;

	UK_TEST_EXPECT(cpus && cpus <= CONFIG_UKPLAT_CPU_MAXCOUNT);
	UK_TEST_EXPECT(cpus <= 64);
	if (!cpus || cpus > CONFIG_UKPLAT_CPU_MAXCOUNT || cpus > 64)
		return;
	uk_semaphore_init(&nested_done, 0);
	for (unsigned int i = 0; i < cpus; i++) {
		__vaddr_t address = UK_PAGING_VADDR_ANY;

		rc = uk_vma_map_anon(vas, &address, 4 * UK_PAGING_PAGE_SIZE,
				     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			UK_CRASH("Cannot reserve nested fault test pages\n");
		probes[i].outer = address;
		probes[i].inner = address + 2 * UK_PAGING_PAGE_SIZE;
		__atomic_store_n(&probes[i].active, true, __ATOMIC_RELEASE);
	}
	for (unsigned int i = 0; i < cpus; i++) {
		struct uk_sched *sched = uk_sched_select_round_robin(local);
		unsigned int cpu = sched->lcpu_idx;
		struct uk_thread *thread;

		UK_TEST_EXPECT(cpu < cpus && !(expected_mask & (1ULL << cpu)));
		if (cpu >= cpus || expected_mask & (1ULL << cpu))
			UK_CRASH("Repeated CPU in nested fault test\n");
		expected_mask |= 1ULL << cpu;
		thread = uk_thread_create_fn1(sched->a, nested_fault_worker,
					      &probes[cpu], sched->a_stack, 0,
					      sched->a_auxstack, 0,
					      sched->a_uktls, false,
					      "nested-fault", &probes[cpu],
					      nested_fault_reaped);
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			UK_CRASH("Cannot allocate nested fault test thread\n");
		rc = uk_sched_thread_add(sched, thread);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			UK_CRASH("Cannot schedule nested fault test thread\n");
	}
	/* Blocking lets the local idle thread collect exited workers too. */
	for (unsigned int i = 0; i < cpus; i++)
		if (uk_semaphore_down_to(&nested_done, wait_ns) == __NSEC_MAX)
			UK_CRASH("Nested fault test timed out\n");
	for (unsigned int i = 0; i < cpus; i++) {
		struct nested_probe *probe = &probes[i];
		__u64 *outer = (__u64 *)probe->outer;
		__u64 *inner = (__u64 *)probe->inner;
		const unsigned int next = UK_PAGING_PAGE_SIZE / sizeof(*outer);
		bool reaped = __atomic_load_n(&probe->reaped, __ATOMIC_ACQUIRE);

		UK_TEST_EXPECT(reaped);
		UK_TEST_EXPECT_SNUM_EQ(probe->faults, 2);
		UK_TEST_EXPECT_SNUM_EQ(outer[0], 7);
		UK_TEST_EXPECT_SNUM_EQ(outer[next], 9);
		UK_TEST_EXPECT_SNUM_EQ(inner[0], 3);
		UK_TEST_EXPECT_SNUM_EQ(inner[next], 3);
		faults += probe->faults;
		__atomic_store_n(&probe->active, false, __ATOMIC_RELEASE);
		rc = uk_vma_unmap(vas, probe->outer,
				  4 * UK_PAGING_PAGE_SIZE, 0);
		UK_TEST_EXPECT_ZERO(rc);
	}
	mask = __atomic_load_n(&nested_cpu_mask, __ATOMIC_ACQUIRE);
	UK_TEST_EXPECT_SNUM_EQ(mask, expected_mask);
	uk_pr_info("NESTED_EXCEPTION_PASS cpus=%u faults=%u mask=%llx\n",
		   cpus, faults, mask);
}

_UK_TESTSUITE(native_nested, NULL);

static void __constructor run_native_nested(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(native_nested));
}
