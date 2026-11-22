/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>
#include <stddef.h>
#include <uk/errptr.h>
#include <uk/plat/spinlock.h>
#include <uk/print.h>
#include <uk/process/events.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/vmem/process.h>

#include "../process.h"

static unsigned int brk_test_vmas(struct uk_vas *vas)
{
	struct uk_vma *vma;
	unsigned int count = 0;
	unsigned long flags;

	ukplat_spin_lock_irqsave(&vas->lock, flags);
	uk_list_for_each_entry(vma, &vas->vma_list, vma_list)
		count++;
	ukplat_spin_unlock_irqrestore(&vas->lock, flags);
	return count;
}

UK_TESTCASE(process_brk_vas, inactive_initialization_and_exit)
{
	struct uk_sched *s = uk_sched_current();
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_vas *saved = uk_vas_get_active(), *vas = NULL;
	struct posix_process *p = NULL;
	struct uk_thread *thread = NULL;
	struct posix_thread *pt;
	struct clone_args args = { .flags = CLONE_VM | CLONE_VFORK };
	struct posix_process_clone_event_data clone_data;
	struct posix_process_ppexit_event_data exit_data;
	unsigned int users = kernel->pt->shared_users;
	unsigned int before;
	__vaddr_t base;
	void *parent_brk = uk_pprocess_current()->brk_ctx.base;
	bool published = false, complete = false;
	int rc;

	UK_TEST_EXPECT_PTR_EQ(saved, kernel);
	if (saved != kernel)
		return;
	rc = uk_vas_create_process(kernel->a, &vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	thread = uk_thread_create_container(s->a, s->a_stack, 0,
					    s->a_auxstack, 0, s->a_uktls, false,
					    "brk-vas", NULL, NULL);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto put_vas;
	p = pprocess_prepare(s->a, thread, uk_thread_current());
	UK_TEST_EXPECT(!PTRISERR(p));
	if (PTRISERR(p)) {
		p = NULL;
		goto release_thread;
	}
	rc = pprocess_bind_prepared_vas(p, vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto release_process;
	before = brk_test_vmas(kernel);
	clone_data = (struct posix_process_clone_event_data) {
		.cl_args = &args,
		.cl_args_len = sizeof(args),
		.child = thread,
		.parent = uk_thread_current(),
		.ppid = ukthread2pid(uk_thread_current()),
		.pid = p->pid,
		.tid = p->pid,
	};
	rc = pprocess_raise_clone_event(&clone_data);
	UK_TEST_EXPECT(rc >= 0);
	if (rc < 0)
		goto release_process;
	base = (__vaddr_t)p->brk_ctx.base;
	UK_TEST_EXPECT(base >= vas->range_start && base < vas->range_end);
	UK_TEST_EXPECT_ZERO(p->brk_ctx.pos);
	UK_TEST_EXPECT_NOT_NULL(uk_vma_find(vas, base));
	UK_TEST_EXPECT_NULL(uk_vma_find(kernel, base));
	UK_TEST_EXPECT_SNUM_EQ(brk_test_vmas(kernel), before);
	UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), saved);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(vas, base, 0x62726bU));
	rc = pprocess_publish(p);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto release_process;
	published = true;
	exit_data.pid = p->pid;
	pprocess_raise_ppexit_event(&exit_data);
	UK_TEST_EXPECT_NULL(p->brk_ctx.base);
	UK_TEST_EXPECT_ZERO(p->brk_ctx.pos);
	UK_TEST_EXPECT_NULL(uk_vma_find(vas, base));
	UK_TEST_EXPECT_SNUM_EQ(brk_test_vmas(kernel), before);
	UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), saved);
	UK_TEST_EXPECT_PTR_EQ(uk_pprocess_current()->brk_ctx.base, parent_brk);
	complete = true;
release_process:
	if (published) {
		pt = uk_thread_uktls_var(thread, pthread_self);
		pprocess_release_pthread(pt);
		pprocess_release(p);
	} else {
		pprocess_abort(p);
	}
release_thread:
	uk_thread_release(thread);
put_vas:
	uk_vas_put(vas);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
	if (complete)
		uk_pr_info("PROCESS_BRK_VAS_PASS cpus=%u inactive=1 released=1\n",
			   uk_sched_started_count());
}

_UK_TESTSUITE(process_brk_vas, NULL);

static void __constructor run_process_brk_vas(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_brk_vas));
}
