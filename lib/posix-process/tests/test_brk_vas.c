/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/crash.h>
#include <uk/errptr.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/semaphore.h>
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

#define BRK_SNAPSHOT_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)

struct brk_snapshot_probe {
	struct uk_mutex *mutex;
	struct uk_semaphore reaped;
	int acquired;
	int recursive;
};

static void brk_snapshot_probe_fn(void *arg)
{
	struct brk_snapshot_probe *p = arg;

	p->acquired = uk_mutex_trylock(p->mutex);
	if (p->acquired) {
		p->recursive = uk_mutex_trylock(p->mutex);
		if (p->recursive)
			uk_mutex_unlock(p->mutex);
		uk_mutex_unlock(p->mutex);
	}
	uk_thread_exit();
}

static void brk_snapshot_probe_reaped(struct uk_thread *thread)
{
	struct brk_snapshot_probe *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

static struct uk_thread *brk_snapshot_raw_thread(void)
{
	struct uk_sched *s = uk_sched_current();

	return uk_thread_create_container(s->a, s->a_stack, 0,
					   s->a_auxstack, 0, s->a_uktls, false,
					   "brk-snapshot", NULL, NULL);
}

UK_TESTCASE(process_brk_vas, clone_preserves_break_and_fresh_mutex)
{
	struct uk_sched *s = uk_sched_current(), *remote = s;
	struct uk_vas *kernel = uk_vas_get_kernel(), *vas = NULL;
	struct posix_process *parent = NULL, *child = NULL;
	struct uk_thread *threads[2] = { NULL, NULL }, *worker = NULL;
	struct clone_args args = { .flags = CLONE_VM | CLONE_VFORK };
	struct posix_process_clone_event_data data;
	struct brk_snapshot_probe probe;
	unsigned int users = kernel->pt->shared_users, i;
	unsigned long irqf;
	__u32 value;
	__vaddr_t base;
	bool locked = false, complete = false;
	int rc;

	if (uk_vas_get_active() != kernel) {
		UK_TEST_EXPECT(false);
		return;
	}
	rc = uk_vas_create_process(kernel->a, &vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	threads[0] = brk_snapshot_raw_thread();
	threads[1] = brk_snapshot_raw_thread();
	UK_TEST_EXPECT(threads[0] && threads[1]);
	if (!threads[0] || !threads[1])
		goto out;
	parent = pprocess_prepare(s->a, threads[0], uk_thread_current());
	UK_TEST_EXPECT(!PTRISERR(parent));
	if (PTRISERR(parent)) {
		parent = NULL;
		goto out;
	}
	rc = pprocess_bind_prepared_vas(parent, vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	data = (struct posix_process_clone_event_data) {
		.cl_args = &args, .cl_args_len = sizeof(args),
		.child = threads[0], .parent = uk_thread_current(),
		.ppid = ukthread2pid(uk_thread_current()),
		.pid = parent->pid, .tid = parent->pid
	};
	rc = pprocess_raise_clone_event(&data);
	UK_TEST_EXPECT(rc >= 0);
	if (rc < 0)
		goto out;
	child = pprocess_prepare(s->a, threads[1], threads[0]);
	UK_TEST_EXPECT(!PTRISERR(child));
	if (PTRISERR(child)) {
		child = NULL;
		goto out;
	}
	UK_TEST_EXPECT_SNUM_EQ(pprocess_clone_prepared_vas(child, child),
			       -EINVAL);
	rc = pprocess_clone_prepared_vas(child, uk_pprocess_current());
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOTSUP);
	irqf = uk_lcpu_save_irqf();
	rc = pprocess_clone_prepared_vas(child, parent);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EWOULDBLOCK);
	UK_TEST_EXPECT_PTR_EQ(child->vas, vas);
	UK_TEST_EXPECT_NULL(child->brk_ctx.base);
	base = (__vaddr_t)parent->brk_ctx.base;
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(vas, base, 0xaaaaU));
	uk_mutex_lock(&parent->brk_ctx.mtx);
	parent->brk_ctx.pos = UK_PAGING_PAGE_SIZE + 17;
	uk_mutex_unlock(&parent->brk_ctx.mtx);
	rc = pprocess_clone_prepared_vas(child, parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT(child->vas != vas);
	UK_TEST_EXPECT_PTR_EQ(child->brk_ctx.base, parent->brk_ctx.base);
	UK_TEST_EXPECT_SNUM_EQ(child->brk_ctx.pos, UK_PAGING_PAGE_SIZE + 17);
	UK_TEST_EXPECT_SNUM_EQ(pprocess_clone_prepared_vas(child, parent),
			       -EBUSY);
	uk_mutex_lock(&parent->brk_ctx.mtx);
	locked = true;
	/* The child mutex is usable while the parent mutex is held. */
	for (i = 0; i < uk_sched_started_count(); i++) {
		remote = uk_sched_select_round_robin(s);
		if (remote != s || uk_sched_started_count() == 1)
			break;
	}
	memset_isr(&probe, 0, sizeof(probe));
	probe.mutex = &child->brk_ctx.mtx;
	uk_semaphore_init(&probe.reaped, 0);
	worker = uk_thread_create_fn1(remote->a, brk_snapshot_probe_fn, &probe,
				      remote->a_stack, 0, remote->a_auxstack, 0,
				      remote->a_uktls, false, "brk-mutex",
				      &probe, brk_snapshot_probe_reaped);
	UK_TEST_EXPECT_NOT_NULL(worker);
	if (!worker)
		goto out;
	rc = uk_sched_thread_add(remote, worker);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		uk_thread_release(worker);
		worker = NULL;
		goto out;
	}
	if (uk_semaphore_down_to(&probe.reaped, BRK_SNAPSHOT_TIMEOUT) ==
	    __NSEC_MAX)
		UK_CRASH("Break snapshot worker was not reaped\n");
	worker = NULL;
	UK_TEST_EXPECT_SNUM_EQ(probe.acquired, 1);
	UK_TEST_EXPECT_SNUM_EQ(probe.recursive, 1);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(child->vas, base, 0xbbbbU));
	UK_TEST_EXPECT_ZERO(uk_vas_load_u32_try(vas, base, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0xaaaa);
	child->brk_ctx.pos++;
	UK_TEST_EXPECT_SNUM_EQ(parent->brk_ctx.pos, UK_PAGING_PAGE_SIZE + 17);
	uk_mutex_unlock(&parent->brk_ctx.mtx);
	locked = false;
	UK_TEST_EXPECT_ZERO(uk_vas_load_u32_try(child->vas, base, &value));
	UK_TEST_EXPECT_SNUM_EQ(value, 0xbbbb);
	complete = true;
out:
	if (locked)
		uk_mutex_unlock(&parent->brk_ctx.mtx);
	pprocess_abort(child);
	pprocess_abort(parent);
	if (threads[1])
		uk_thread_release(threads[1]);
	if (threads[0])
		uk_thread_release(threads[0]);
	uk_vas_put(vas);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
	if (complete)
		uk_pr_info("PROCESS_BRK_VAS_PASS clone=1 fresh_mutex=1 remote=%u\n",
			   remote != s);
}

_UK_TESTSUITE(process_brk_vas, NULL);

static void __constructor run_process_brk_vas(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_brk_vas));
}
