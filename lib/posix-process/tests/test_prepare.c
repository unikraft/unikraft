/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <uk/alloc.h>
#include <uk/arch/util.h>
#include <uk/crash.h>
#include <uk/errptr.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/test.h>
#include <uk/vmem/process.h>
#include <uk/vmem/vma_types.h>

#include "../process.h"
#include "../signal/signal.h"

#define PREPARE_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)
#define PREPARE_VALUE 0x70726570U

struct prepare_alloc {
	struct uk_alloc *real;
	unsigned int attempts;
	unsigned int allocated;
	unsigned int freed;
	unsigned int fail_at;
};

static void *prepare_malloc(struct uk_alloc *a, __sz size)
{
	struct prepare_alloc *p = (void *)a->priv;
	void *ptr;

	if (++p->attempts == p->fail_at)
		return NULL;
	ptr = uk_malloc(p->real, size);
	if (ptr)
		p->allocated++;
	return ptr;
}

static void *prepare_calloc(struct uk_alloc *a, __sz count, __sz size)
{
	void *ptr;

	if (size && count > ((__sz)-1) / size)
		return NULL;
	ptr = prepare_malloc(a, count * size);
	if (ptr)
		memset_isr(ptr, 0, count * size);
	return ptr;
}

static void prepare_free(struct uk_alloc *a, void *ptr)
{
	struct prepare_alloc *p = (void *)a->priv;

	if (ptr) {
		p->freed++;
		uk_free(p->real, ptr);
	}
}

static struct uk_alloc *prepare_allocator(void)
{
	struct uk_alloc *real = uk_sched_current()->a;
	struct uk_alloc *a;
	struct prepare_alloc *p;

	a = uk_malloc(real, sizeof(*a) + sizeof(*p));
	if (!a)
		return NULL;
	memset_isr(a, 0, sizeof(*a) + sizeof(*p));
	p = (void *)a->priv;
	p->real = real;
	a->malloc = prepare_malloc;
	a->calloc = prepare_calloc;
	a->free = prepare_free;
	return a;
}

static unsigned int prepare_children(struct posix_process *parent)
{
	struct posix_process *p;
	unsigned int count = 0;
	unsigned long flags = pprocess_lock_acquire();

	uk_list_for_each_entry(p, &parent->children, child_list_entry)
		count++;
	pprocess_lock_release(flags);
	return count;
}

static unsigned int prepare_processes(void)
{
	unsigned int count = 0;
	unsigned long flags = pprocess_lock_acquire();

	for (unsigned int i = 1; i < TIDMAP_SIZE; i++)
		count += !!pid_process[i];
	pprocess_lock_release(flags);
	return count;
}

static unsigned long prepare_aliases(void)
{
	struct uk_pagetable *pt = uk_vas_get_kernel()->pt;
	unsigned long frames = 0;

	for (unsigned int level = 0; level < UK_PAGING_PT_LEVELS; level++)
		frames += pt->nr_lx_pages[level] *
			(UK_PAGING_PAGE_Lx_SIZE(level) / UK_PAGING_PAGE_SIZE);
	return frames;
}

static struct uk_thread *prepare_raw_thread(void)
{
	struct uk_sched *s = uk_sched_current();

	return uk_thread_create_container(s->a, s->a_stack, 0,
					   s->a_auxstack, 0, s->a_uktls, false,
					   "prepared", NULL, NULL);
}

static int prepare_clone_event(struct posix_process *p, struct uk_thread *child)
{
	struct clone_args args = { 0 };
	struct posix_process_clone_event_data data = {
		.cl_args = &args,
		.cl_args_len = sizeof(args),
		.child = child,
		.parent = uk_thread_current(),
		.ppid = ukthread2pid(uk_thread_current()),
		.pid = p->pid,
		.tid = p->pid,
	};
	int rc = pprocess_raise_clone_event(&data);

	return rc < 0 ? rc : 0;
}

UK_TESTCASE(process_prepare, unpublished_clone_cycles)
{
	struct uk_alloc *a = prepare_allocator();
	struct prepare_alloc *alloc;
	struct uk_thread *thread;
	struct posix_process *p;
	struct posix_process *parent = uk_pprocess_current();
	unsigned int children = prepare_children(parent);
	unsigned int processes = prepare_processes();
	unsigned long first_free = 0, first_aliases = 0, free_bytes, aliases;
	pid_t pid;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	for (unsigned int cycle = 1; cycle <= 64; cycle++) {
		alloc->attempts = 0;
		alloc->allocated = 0;
		alloc->freed = 0;
		thread = prepare_raw_thread();
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			break;
		p = pprocess_prepare(a, thread, uk_thread_current());
		UK_TEST_EXPECT(!PTRISERR(p));
		if (PTRISERR(p)) {
			uk_thread_release(thread);
			break;
		}
		pid = p->pid;
		UK_TEST_EXPECT_NULL(pid2pprocess(pid));
		UK_TEST_EXPECT_NULL(tid2pthread(pid));
		UK_TEST_EXPECT_SNUM_EQ(prepare_children(parent), children);
		rc = prepare_clone_event(p, thread);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_NULL(pid2pprocess(pid));
		UK_TEST_EXPECT_NULL(tid2pthread(pid));
		pprocess_abort(p);
		UK_TEST_EXPECT_NULL(uk_thread_uktls_var(thread, pthread_self));
		uk_thread_release(thread);
		UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
		UK_TEST_EXPECT_SNUM_EQ(prepare_children(parent), children);
		UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
		free_bytes = uk_vas_get_kernel()->pt->fa->free_memory;
		aliases = prepare_aliases();
		if (cycle == 1) {
			first_free = free_bytes;
			first_aliases = aliases;
		}
		UK_TEST_EXPECT_SNUM_EQ(free_bytes, first_free);
		UK_TEST_EXPECT_SNUM_EQ(aliases, first_aliases);
		uk_pr_info("PROCESS_PREPARE_CYCLE cycle=%u allocated=%u freed=%u free_bytes=%lu alias_frames=%lu\n",
			   cycle, alloc->allocated, alloc->freed,
			   free_bytes, aliases);
		if (rc)
			break;
	}
	uk_free(uk_sched_current()->a, a);
}

UK_TESTCASE(process_prepare, allocation_failure_keeps_ids_hidden)
{
	struct uk_alloc *a = prepare_allocator();
	struct prepare_alloc *alloc;
	struct uk_thread *thread;
	struct posix_process *p;
	struct posix_process *parent = uk_pprocess_current();
	unsigned int children = prepare_children(parent);
	unsigned int processes = prepare_processes();
	unsigned int calls;
	unsigned int observed_children;
	void *binding;
	const unsigned int rounds = CONFIG_LIBPOSIX_PROCESS_MAX_PID + 1;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	thread = prepare_raw_thread();
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto free_allocator;
	p = pprocess_prepare(a, thread, uk_thread_current());
	UK_TEST_EXPECT(!PTRISERR(p));
	if (PTRISERR(p))
		goto release_thread;
	calls = alloc->attempts;
	pprocess_abort(p);
	for (unsigned int fail = 1; fail <= calls; fail++) {
		for (unsigned int round = 0; round < rounds; round++) {
			alloc->attempts = 0;
			alloc->allocated = 0;
			alloc->freed = 0;
			alloc->fail_at = fail;
			p = pprocess_prepare(a, thread, uk_thread_current());
			UK_TEST_EXPECT_SNUM_EQ(PTR2ERR(p), -ENOMEM);
			if (!PTRISERR(p)) {
				pprocess_abort(p);
				goto release_thread;
			}
			binding = uk_thread_uktls_var(thread, pthread_self);
			UK_TEST_EXPECT_NULL(binding);
			UK_TEST_EXPECT_SNUM_EQ(alloc->attempts, fail);
			UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
			observed_children = prepare_children(parent);
			UK_TEST_EXPECT_SNUM_EQ(observed_children, children);
			UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
		}
	}
	uk_pr_info("PROCESS_PREPARE_PASS rollback calls=%u rounds=%u\n",
		   calls, rounds);
release_thread:
	uk_thread_release(thread);
free_allocator:
	uk_free(uk_sched_current()->a, a);
}

struct prepare_run {
	struct uk_vas *vas;
	struct posix_process *process;
	struct uk_sched *caller;
	struct uk_semaphore finish;
	struct uk_semaphore reaped;
	__vaddr_t address;
	bool started;
	bool visible;
	bool active;
	bool remote;
	bool after_release;
};

static void prepare_run_thread(void *arg)
{
	struct prepare_run *p = arg;
	struct posix_thread *pt = uk_pthread_current();
	__u32 *value = (__u32 *)p->address;

	p->visible = pt && pt->process == p->process &&
		pid2pprocess(pt->tid) == p->process &&
		tid2pthread(pt->tid) == pt;
	p->active = uk_vas_get_active() == p->vas &&
		uk_thread_current()->vas_ref == p->vas &&
		!uk_lcpu_irqs_disabled();
	p->remote = uk_sched_current() != p->caller;
	p->active = p->active &&
		__atomic_load_n(value, __ATOMIC_RELAXED) == PREPARE_VALUE;
	__atomic_store_n(value, PREPARE_VALUE + 1, __ATOMIC_RELAXED);
	__atomic_store_n(&p->started, true, __ATOMIC_RELEASE);
	uk_semaphore_down(&p->finish);
	p->after_release = !uk_pthread_current() &&
		uk_vas_get_active() == p->vas &&
		__atomic_load_n(value, __ATOMIC_RELAXED) == PREPARE_VALUE + 1;
	uk_thread_exit();
}

static void prepare_run_reaped(struct uk_thread *thread)
{
	struct prepare_run *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

static void prepare_wait(bool *flag)
{
	__nsec limit = ukplat_monotonic_clock() + PREPARE_TIMEOUT;

	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (ukplat_monotonic_clock() >= limit)
			UK_CRASH("Prepared process thread made no progress\n");
		uk_sched_yield();
	}
}

UK_TESTCASE(process_prepare, publish_owned_vas_lifetime)
{
	struct uk_sched *local = uk_sched_current(), *target = local;
	struct prepare_run probe = { .caller = local };
	struct posix_process *p = NULL;
	struct posix_thread *pt;
	struct uk_thread *thread = NULL;
	struct uk_vas *vas = NULL;
	struct uk_vas *kernel = uk_vas_get_kernel();
	unsigned int cpus = uk_sched_started_count();
	unsigned int children = prepare_children(uk_pprocess_current());
	unsigned int observed_children;
	unsigned int users = kernel->pt->shared_users;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__nsec limit;
	__nsec elapsed;
	pid_t pid;
	int rc;

	for (unsigned int i = 0; i < cpus && target == local; i++)
		target = uk_sched_select_round_robin(local);
	uk_semaphore_init(&probe.finish, 0);
	uk_semaphore_init(&probe.reaped, 0);
	rc = uk_vas_create_process(kernel->a, &vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	probe.vas = vas;
	rc = uk_vma_map_anon(vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto put_vas;
	probe.address = address;
	rc = uk_vas_store_u32(vas, address, PREPARE_VALUE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto put_vas;
	thread = uk_thread_create_fn1(target->a, prepare_run_thread, &probe,
				      target->a_stack, 0, target->a_auxstack, 0,
				      target->a_uktls, false, "prepared-run",
				      &probe, prepare_run_reaped);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto put_vas;
	uk_thread_set_blocked(thread);
	p = pprocess_prepare(local->a, thread, uk_thread_current());
	UK_TEST_EXPECT(!PTRISERR(p));
	if (PTRISERR(p)) {
		p = NULL;
		goto release_thread;
	}
	probe.process = p;
	pid = p->pid;
	pt = uk_thread_uktls_var(thread, pthread_self);
	UK_TEST_EXPECT_ZERO(prepare_clone_event(p, thread));
	rc = pprocess_bind_prepared_vas(p, vas);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort_process;
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&vas->refs), 3);
	rc = uk_sched_thread_add(target, thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort_process;
	for (unsigned int i = 0; i < 4; i++)
		uk_sched_yield();
	UK_TEST_EXPECT(!__atomic_load_n(&probe.started, __ATOMIC_ACQUIRE));
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	UK_TEST_EXPECT_NULL(tid2pthread(pid));
	rc = pprocess_publish(p);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		UK_TEST_EXPECT_ZERO(uk_sched_thread_remove(thread));
		goto abort_process;
	}
	UK_TEST_EXPECT_PTR_EQ(pid2pprocess(pid), p);
	UK_TEST_EXPECT_PTR_EQ(tid2pthread(pid), pt);
	observed_children = prepare_children(uk_pprocess_current());
	UK_TEST_EXPECT_SNUM_EQ(observed_children, children + 1);
	for (unsigned int i = 0; i < 4; i++)
		uk_sched_yield();
	UK_TEST_EXPECT(!__atomic_load_n(&probe.started, __ATOMIC_ACQUIRE));
	uk_thread_wake(thread);
	prepare_wait(&probe.started);
	UK_TEST_EXPECT(probe.visible);
	UK_TEST_EXPECT(probe.active);
	UK_TEST_EXPECT(probe.remote == (cpus > 1));
	/* The raw child waits on finish and uses no POSIX state afterwards. */
	pprocess_release_pthread(pt);
	pprocess_release(p);
	p = NULL;
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	observed_children = prepare_children(uk_pprocess_current());
	UK_TEST_EXPECT_SNUM_EQ(observed_children, children);
	UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&vas->refs), 2);
	uk_semaphore_up(&probe.finish);
	/* Block so the local idle thread can collect the exited child. */
	elapsed = uk_semaphore_down_to(&probe.reaped, PREPARE_TIMEOUT);
	if (elapsed == __NSEC_MAX)
		UK_CRASH("Prepared process thread was not reaped\n");
	thread = NULL;
	limit = ukplat_monotonic_clock() + PREPARE_TIMEOUT;
	while (uk_refcount_read(&vas->refs) != 1) {
		if (ukplat_monotonic_clock() >= limit)
			UK_CRASH("Prepared process thread retained its VAS\n");
		uk_sched_yield();
	}
	UK_TEST_EXPECT(probe.after_release);
	uk_pr_info("PROCESS_PREPARE_PASS published cpus=%u remote=%u\n",
		   cpus, probe.remote);
	goto put_vas;
abort_process:
	pprocess_abort(p);
release_thread:
	uk_thread_release(thread);
put_vas:
	uk_vas_put(vas);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
}

UK_TESTCASE(process_prepare, pid_survives_thread_descriptor_release)
{
	struct uk_alloc *a = prepare_allocator();
	struct prepare_alloc *alloc;
	struct uk_thread *parent = uk_thread_current();
	struct uk_thread *held_thread = NULL, *other_thread = NULL;
	struct posix_process *held = NULL, *other;
	struct posix_thread *pt;
	unsigned int before = prepare_processes();
	unsigned int rounds = CONFIG_LIBPOSIX_PROCESS_MAX_PID + 1;
	pid_t held_pid;
	bool complete = false, reused = false;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	held_thread = prepare_raw_thread();
	other_thread = prepare_raw_thread();
	UK_TEST_EXPECT(held_thread && other_thread);
	if (!held_thread || !other_thread)
		goto release_threads;
	held = pprocess_prepare(a, held_thread, parent);
	UK_TEST_EXPECT(!PTRISERR(held));
	if (PTRISERR(held)) {
		held = NULL;
		goto release_threads;
	}
	held_pid = held->pid;
	rc = pprocess_publish(held);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		pprocess_abort(held);
		held = NULL;
		goto release_threads;
	}
	pt = uk_thread_uktls_var(held_thread, pthread_self);
	pprocess_release_pthread(pt);
	/* The PID stays visible until reaping after its leader TID is gone. */
	UK_TEST_EXPECT_NULL(tid2pthread(held_pid));
	UK_TEST_EXPECT_PTR_EQ(pid2pprocess(held_pid), held);
	for (unsigned int round = 0; round < rounds; round++) {
		other = pprocess_prepare(a, other_thread, parent);
		UK_TEST_EXPECT(!PTRISERR(other));
		if (PTRISERR(other))
			goto release_held;
		UK_TEST_EXPECT(other->pid != held_pid);
		UK_TEST_EXPECT_PTR_EQ(pid2pprocess(held_pid), held);
		pprocess_abort(other);
	}
	complete = true;
release_held:
	pprocess_release(held);
	held = NULL;
	UK_TEST_EXPECT_NULL(pid2pprocess(held_pid));
	if (complete) {
		for (unsigned int round = 0; round < rounds; round++) {
			other = pprocess_prepare(a, other_thread, parent);
			UK_TEST_EXPECT(!PTRISERR(other));
			if (PTRISERR(other))
				break;
			reused = other->pid == held_pid;
			pprocess_abort(other);
			if (reused)
				break;
		}
		UK_TEST_EXPECT(reused);
		uk_pr_info("PROCESS_PREPARE_PASS pid_retention rounds=%u reused=%u\n",
			   rounds, reused);
	}
release_threads:
	if (other_thread)
		uk_thread_release(other_thread);
	if (held_thread)
		uk_thread_release(held_thread);
	UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
	UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), before);
	uk_free(uk_sched_current()->a, a);
}

_UK_TESTSUITE(process_prepare, NULL);

static void __constructor run_process_prepare(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_prepare));
}
