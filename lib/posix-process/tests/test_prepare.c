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
#include <uk/process.h>
#include <uk/process/prepared.h>
#if CONFIG_LIBPOSIX_FDTAB_MULTITAB && !CONFIG_LIBVFSCORE
#include <fcntl.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/posix-fdtab.h>
#endif
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
#if CONFIG_LIBPOSIX_FDTAB_MULTITAB && !CONFIG_LIBVFSCORE
	bool check_fds;
	bool kept_open;
	bool clo_closed;
	int kept_fd;
	int clo_fd;
#endif
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
#if CONFIG_LIBPOSIX_FDTAB_MULTITAB && !CONFIG_LIBVFSCORE
	if (p->check_fds) {
		p->kept_open = uk_fdtab_getflags(p->kept_fd) >= 0;
		p->clo_closed = uk_fdtab_getflags(p->clo_fd) < 0;
	}
#endif
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

#if CONFIG_LIBPOSIX_FDTAB_MULTITAB && !CONFIG_LIBVFSCORE
static struct uk_thread *prepare_public_raw_thread(void)
{
	struct uk_sched *s = uk_sched_current();

	/* The loader will supply a private user stack after preparation. */
	return uk_thread_create_container(s->a, NULL, 0,
					   s->a_auxstack, 0, s->a_uktls, false,
					   "public-raw", NULL, NULL);
}

UK_TESTCASE(process_prepare, public_spawn_cycles)
{
	struct uk_vas *kernel = uk_vas_get_kernel(), *vas = NULL;
	struct uk_alloc *a = prepare_allocator();
	struct prepare_alloc *alloc;
	struct uk_thread *thread = NULL;
	struct posix_thread *pt;
	unsigned int children = prepare_children(uk_pprocess_current());
	unsigned int processes = prepare_processes();
	unsigned int users = kernel->pt->shared_users;
	unsigned int cycle;
	unsigned long free_bytes = 0;
	pid_t pid;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	UK_TEST_EXPECT_ZERO(uk_posix_process_abort_prepared(NULL));
	UK_TEST_EXPECT_SNUM_EQ(uk_posix_process_publish_prepared(NULL),
			       -EINVAL);
	UK_TEST_EXPECT_SNUM_EQ(uk_posix_process_prepare_spawn(a, NULL, &vas),
			       -EINVAL);
	UK_TEST_EXPECT_NULL(vas);
	for (cycle = 0; cycle < 66; cycle++) {
		thread = prepare_public_raw_thread();
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			goto out;
		UK_TEST_EXPECT_NULL(thread->_mem.stack);
		pid = uk_posix_process_prepare_spawn(a, thread, &vas);
		UK_TEST_EXPECT(pid > 0);
		if (pid <= 0)
			goto out;
		pt = uk_thread_uktls_var(thread, pthread_self);
		UK_TEST_EXPECT_NOT_NULL(pt);
		UK_TEST_EXPECT_PTR_EQ(pt->process->vas, vas);
		UK_TEST_EXPECT_PTR_EQ(thread->vas, vas);
		UK_TEST_EXPECT(uk_vas_is_owned(vas));
		UK_TEST_EXPECT_SNUM_EQ(uk_refcount_read(&vas->refs), 3);
		UK_TEST_EXPECT_SNUM_EQ(pt->process->exit_signal, SIGCHLD);
		UK_TEST_EXPECT_NULL(pid2pprocess(pid));
		UK_TEST_EXPECT_NULL(tid2pthread(pid));
#if CONFIG_LIBPOSIX_PROCESS_BRK
		UK_TEST_EXPECT_NOT_NULL(pt->process->brk_ctx.base);
		UK_TEST_EXPECT_ZERO(pt->process->brk_ctx.pos);
		{
			__vaddr_t heap = (__vaddr_t)pt->process->brk_ctx.base;

			UK_TEST_EXPECT_NOT_NULL(uk_vma_find(vas, heap));
		}
#endif
		UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), kernel);
		UK_TEST_EXPECT_ZERO(uk_posix_process_abort_prepared(thread));
		uk_thread_release(thread);
		thread = NULL;
		uk_vas_put(vas);
		vas = NULL;
		UK_TEST_EXPECT_SNUM_EQ(prepare_children(uk_pprocess_current()),
				       children);
		UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
		UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
		if (cycle == 1)
			free_bytes = kernel->pt->fa->free_memory;
		if (cycle >= 2) {
			UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory,
					       free_bytes);
			uk_pr_info("PROCESS_SPAWN_CYCLE cycle=%u free_bytes=%lu shared_users=%u\n",
				   cycle - 1, free_bytes, users);
		}
	}
	uk_pr_info("PROCESS_SPAWN_PASS cycles=64\n");
out:
	if (thread)
		uk_thread_release(thread);
	uk_vas_put(vas);
	uk_free(uk_sched_current()->a, a);
}

static struct {
	struct uk_thread *child;
	bool seen;
	bool owned;
	bool unpublished;
	bool heap;
	bool clear_actions;
} prepare_spawn_failure;

static int prepare_spawn_fail_event(void *arg)
{
	struct posix_process_clone_event_data *data = arg;
	struct posix_thread *pt;
	struct posix_process *p;

	if (!prepare_spawn_failure.child ||
	    data->child != prepare_spawn_failure.child)
		return UK_EVENT_HANDLED_CONT;
	pt = uk_thread_uktls_var(data->child, pthread_self);
	p = pt->process;
	prepare_spawn_failure.seen = true;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	prepare_spawn_failure.clear_actions =
		!!(data->cl_args->flags & CLONE_CLEAR_SIGHAND);
#else
	prepare_spawn_failure.clear_actions = true;
#endif
	prepare_spawn_failure.owned = uk_vas_is_owned(p->vas);
	prepare_spawn_failure.unpublished = !pid2pprocess(p->pid) &&
		!tid2pthread(pt->tid);
#if CONFIG_LIBPOSIX_PROCESS_BRK
	prepare_spawn_failure.heap = p->brk_ctx.base &&
		uk_vma_find(p->vas, (__vaddr_t)p->brk_ctx.base);
#else
	prepare_spawn_failure.heap = true;
#endif
	return -ENOMEM;
}

POSIX_PROCESS_CLONE_HANDLER(0, prepare_spawn_fail_event);

UK_TESTCASE(process_prepare, public_spawn_failure_cleanup)
{
	struct uk_vas *kernel = uk_vas_get_kernel(), *vas = NULL;
	struct uk_alloc *a = prepare_allocator();
	struct prepare_alloc *alloc;
	struct uk_thread *thread;
	unsigned int users = kernel->pt->shared_users;
	unsigned int processes = prepare_processes(), cycle;
	unsigned int children = prepare_children(uk_pprocess_current());
	unsigned long free_bytes = 0;
	pid_t result;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	alloc = (void *)a->priv;
	for (cycle = 0; cycle <= 32; cycle++) {
		thread = prepare_public_raw_thread();
		UK_TEST_EXPECT_NOT_NULL(thread);
		if (!thread)
			goto out;
		UK_TEST_EXPECT_NULL(thread->_mem.stack);
		memset_isr(&prepare_spawn_failure, 0,
			   sizeof(prepare_spawn_failure));
		prepare_spawn_failure.child = thread;
		result = uk_posix_process_prepare_spawn(a, thread, &vas);
		prepare_spawn_failure.child = NULL;
		UK_TEST_EXPECT_SNUM_EQ(result, -ENOMEM);
		UK_TEST_EXPECT_NULL(vas);
		UK_TEST_EXPECT(prepare_spawn_failure.seen);
		UK_TEST_EXPECT(prepare_spawn_failure.owned);
		UK_TEST_EXPECT(prepare_spawn_failure.unpublished);
		UK_TEST_EXPECT(prepare_spawn_failure.heap);
		UK_TEST_EXPECT(prepare_spawn_failure.clear_actions);
		UK_TEST_EXPECT_NULL(uk_thread_uktls_var(thread, pthread_self));
		UK_TEST_EXPECT(uk_vas_is_owned(thread->vas));
		UK_TEST_EXPECT_PTR_EQ(uk_vas_get_active(), kernel);
		/* Drop the last VAS pin retained for UKTLS/storage teardown. */
		uk_thread_release(thread);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
		UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
		UK_TEST_EXPECT_SNUM_EQ(prepare_children(uk_pprocess_current()),
				       children);
		UK_TEST_EXPECT_SNUM_EQ(alloc->allocated, alloc->freed);
		if (!cycle)
			free_bytes = kernel->pt->fa->free_memory;
		else
			UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory,
					       free_bytes);
	}
	uk_pr_info("PROCESS_SPAWN_PASS rollback cycles=32 after_heap=1\n");
out:
	prepare_spawn_failure.child = NULL;
	uk_free(uk_sched_current()->a, a);
}

struct prepare_public_file {
	struct uk_file file;
	uk_file_refcnt refs;
	struct uk_file_state state;
	unsigned int released;
};

static void prepare_public_file_release(const struct uk_file *file, int what)
{
	struct prepare_public_file *f = file->node;

	__atomic_fetch_or(&f->released, what, __ATOMIC_RELEASE);
}

UK_TESTCASE(process_prepare, public_publish_owned_vas)
{
	struct uk_sched *local = uk_sched_current(), *target = local;
	struct prepare_run probe = { .caller = local };
	struct prepare_public_file file = { 0 };
	struct uk_vas *kernel = uk_vas_get_kernel(), *vas = NULL;
	struct uk_thread *thread;
	struct posix_thread *pt;
	struct posix_process *p;
	unsigned int cpus = uk_sched_started_count();
	unsigned int users = kernel->pt->shared_users;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	__nsec limit;
	pid_t pid;
	int rc, kept_fd = -1, clo_fd = -1;

	for (unsigned int i = 0; i < cpus && target == local; i++)
		target = uk_sched_select_round_robin(local);
	uk_semaphore_init(&probe.finish, 0);
	uk_semaphore_init(&probe.reaped, 0);
	file.refs = UK_FILE_REFCNT_INIT_VALUE(file.refs);
	file.state = UK_FILE_STATE_INIT_VALUE(file.state);
	file.file = (struct uk_file) {
		.vol = &file, .node = &file, .ops = &uk_file_nops,
		.refcnt = &file.refs, .state = &file.state,
		._release = prepare_public_file_release
	};
	kept_fd = uk_fdtab_open(&file.file, O_RDONLY);
	clo_fd = uk_fdtab_open(&file.file, O_RDONLY | O_CLOEXEC);
	UK_TEST_EXPECT(kept_fd >= 0 && clo_fd >= 0);
	if (kept_fd < 0 || clo_fd < 0)
		goto close_fds;
	probe.check_fds = true;
	probe.kept_fd = kept_fd;
	probe.clo_fd = clo_fd;
	thread = uk_thread_create_fn1(target->a, prepare_run_thread, &probe,
				      target->a_stack, 0, target->a_auxstack, 0,
				      target->a_uktls, false, "public-spawn",
				      &probe, prepare_run_reaped);
	UK_TEST_EXPECT_NOT_NULL(thread);
	if (!thread)
		goto close_fds;
	uk_thread_set_blocked(thread);
	pid = uk_posix_process_prepare_spawn(local->a, thread, &vas);
	UK_TEST_EXPECT(pid > 0);
	if (pid <= 0)
		goto release_thread;
	UK_TEST_EXPECT(uk_fdtab_getflags(kept_fd) >= 0);
	UK_TEST_EXPECT(uk_fdtab_getflags(clo_fd) >= 0);
	pt = uk_thread_uktls_var(thread, pthread_self);
	p = pt->process;
	probe.process = p;
	probe.vas = vas;
	rc = uk_vma_map_anon(vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	if (!rc)
		rc = uk_vas_store_u32(vas, address, PREPARE_VALUE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort;
	probe.address = address;
	rc = uk_sched_thread_add(target, thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort;
	UK_TEST_EXPECT_SNUM_EQ(uk_posix_process_abort_prepared(thread), -EBUSY);
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	rc = uk_posix_process_publish_prepared(thread);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		UK_TEST_EXPECT_ZERO(uk_sched_thread_remove(thread));
		goto abort;
	}
	rc = uk_posix_process_publish_prepared(thread);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EBUSY);
	UK_TEST_EXPECT_SNUM_EQ(uk_posix_process_abort_prepared(thread), -EBUSY);
	for (unsigned int i = 0; i < 4; i++)
		uk_sched_yield();
	UK_TEST_EXPECT(!__atomic_load_n(&probe.started, __ATOMIC_ACQUIRE));
	uk_thread_wake(thread);
	prepare_wait(&probe.started);
	UK_TEST_EXPECT(probe.visible);
	UK_TEST_EXPECT(probe.active);
	UK_TEST_EXPECT(probe.kept_open);
	UK_TEST_EXPECT(probe.clo_closed);
	UK_TEST_EXPECT(probe.remote == (cpus > 1));
	pprocess_release_pthread(pt);
	pprocess_release(p);
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	uk_semaphore_up(&probe.finish);
	if (uk_semaphore_down_to(&probe.reaped, PREPARE_TIMEOUT) == __NSEC_MAX)
		UK_CRASH("Public spawn thread was not reaped\n");
	thread = NULL;
	limit = ukplat_monotonic_clock() + PREPARE_TIMEOUT;
	while (uk_refcount_read(&vas->refs) != 1) {
		if (ukplat_monotonic_clock() >= limit)
			UK_CRASH("Public spawn thread retained its VAS\n");
		uk_sched_yield();
	}
	UK_TEST_EXPECT(probe.after_release);
	uk_pr_info("PROCESS_SPAWN_PASS published remote=%u cloexec=1\n",
		   probe.remote);
	goto put_vas;
abort:
	UK_TEST_EXPECT_ZERO(uk_posix_process_abort_prepared(thread));
release_thread:
	uk_thread_release(thread);
put_vas:
	uk_vas_put(vas);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
close_fds:
	if (kept_fd >= 0)
		UK_TEST_EXPECT_ZERO(uk_sys_close(kept_fd));
	if (clo_fd >= 0)
		UK_TEST_EXPECT_ZERO(uk_sys_close(clo_fd));
	uk_file_release(&file.file);
	UK_TEST_EXPECT_SNUM_EQ(file.released,
			       UK_FILE_RELEASE_RES | UK_FILE_RELEASE_OBJ);
	if (file.released != (UK_FILE_RELEASE_RES | UK_FILE_RELEASE_OBJ))
		UK_CRASH("Prepared spawn retained a test file reference\n");
}

#if CONFIG_LIBPOSIX_PROCESS_FORK
static bool prepare_kernel_fork_rejected(void)
{
	struct ukarch_execenv *env;
	struct clone_args args = { .exit_signal = SIGCHLD };
	int rc;

	env = uk_zalloc(uk_sched_current()->a, sizeof(*env));
	if (!env)
		return false;
	uk_lcpu_regs_set(env->regs, SP, (__uptr)(&env));
	uk_lcpu_sysctx_set(env->sysctx, TLSP, uk_thread_current()->uktlsp);
	rc = pprocess_fork(&args, sizeof(args), env);
	uk_free(uk_sched_current()->a, env);
	return rc == -ENOTSUP;
}

static bool prepare_kernel_exec_rejected(void)
{
	struct ukarch_execenv *env;
	int rc;

	env = uk_zalloc(uk_sched_current()->a, sizeof(*env));
	if (!env)
		return false;
	uk_lcpu_regs_set(env->regs, SP, (__uptr)(&env));
	uk_lcpu_sysctx_set(env->sysctx, TLSP, uk_thread_current()->uktlsp);
	rc = pprocess_execve_owned(env, NULL, NULL, NULL);
	uk_free(uk_sched_current()->a, env);
	return rc == -ENOTSUP;
}

UK_TESTCASE(process_prepare, resident_exec_allocator)
{
	const __sz alignment = 2 * UK_PAGING_PAGE_SIZE;
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct uk_alloc *a = kernel->a;
	unsigned long before = kernel->pt->fa->free_memory;
	unsigned int cycles = 0;
	bool zeroed, preserved;
	unsigned char *p, *next;
	void *aligned;
	int rc;

	for (; cycles < 16; cycles++) {
		p = uk_calloc(a, 32, 1);
		UK_TEST_EXPECT_NOT_NULL(p);
		if (!p)
			break;
		zeroed = true;
		for (unsigned int i = 0; i < 32; i++)
			zeroed &= p[i] == 0;
		UK_TEST_EXPECT(zeroed);
		memset(p, 0x5a, 32);
		next = uk_realloc(a, p, 128);
		UK_TEST_EXPECT_NOT_NULL(next);
		if (!next) {
			uk_free(a, p);
			break;
		}
		preserved = true;
		for (unsigned int i = 0; i < 32; i++)
			preserved &= next[i] == 0x5a;
		UK_TEST_EXPECT(preserved);
		uk_free(a, next);
		aligned = NULL;
		rc = uk_posix_memalign(a, &aligned, alignment, 128);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_NOT_NULL(aligned);
		if (rc || !aligned)
			break;
		UK_TEST_EXPECT(IS_ALIGNED((__uptr)aligned, alignment));
		memset(aligned, 0xa5, 128);
		uk_free(a, aligned);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, before);
	}
	UK_TEST_EXPECT_SNUM_EQ(cycles, 16);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, before);
	uk_pr_info("EXEC_ALLOC_PASS cycles=16 zero=1 resize=1 aligned=1\n");
}

UK_TESTCASE(process_prepare, exec_rejects_kernel_context)
{
	struct posix_process *parent = uk_pprocess_current();
	struct uk_vas *kernel = uk_vas_get_kernel();
	unsigned int children = prepare_children(parent);
	unsigned int processes = prepare_processes();
	unsigned long aliases = prepare_aliases();
	unsigned long free_bytes = kernel->pt->fa->free_memory;

	UK_TEST_EXPECT(prepare_kernel_exec_rejected());
	UK_TEST_EXPECT_SNUM_EQ(prepare_children(parent), children);
	UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
	UK_TEST_EXPECT_SNUM_EQ(prepare_aliases(), aliases);
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, free_bytes);
	uk_pr_info("PROCESS_EXEC_REJECT_PASS kernel_parent=1\n");
}

UK_TESTCASE(process_prepare, fork_rejects_kernel_context)
{
	struct posix_process *parent = uk_pprocess_current();
	unsigned int children = prepare_children(parent);
	unsigned int processes = prepare_processes();
	unsigned long aliases = prepare_aliases();

	UK_TEST_EXPECT(prepare_kernel_fork_rejected());
	UK_TEST_EXPECT_SNUM_EQ(prepare_children(parent), children);
	UK_TEST_EXPECT_SNUM_EQ(prepare_processes(), processes);
	UK_TEST_EXPECT_SNUM_EQ(prepare_aliases(), aliases);
	uk_pr_info("PROCESS_FORK_REJECT_PASS kernel_parent=1\n");
}
#endif

struct prepare_pthread_run {
	struct uk_vas *vas;
	struct posix_process *process;
	struct uk_sched *target;
	struct uk_semaphore child_reaped;
#if CONFIG_LIBPOSIX_PROCESS_FORK
	bool kernel_fork_rejected;
	bool kernel_exec_rejected;
#endif
	struct uk_semaphore done;
	struct uk_semaphore finish;
	struct uk_semaphore reaped;
	__vaddr_t address;
	unsigned int cycles;
	bool bound;
	bool active;
	bool visible;
	bool balanced;
	bool remote;
};

static void prepare_pthread_refs(struct uk_vas *vas, unsigned int refs)
{
	__nsec limit = ukplat_monotonic_clock() + PREPARE_TIMEOUT;

	while (uk_refcount_read(&vas->refs) != refs) {
		if (ukplat_monotonic_clock() >= limit)
			UK_CRASH("Process pthread retained its VAS\n");
		uk_sched_yield();
	}
}

static void prepare_pthread_child(void *arg)
{
	struct prepare_pthread_run *p = arg;
	struct posix_thread *pt = uk_pthread_current();
	__u32 *value = (__u32 *)p->address;
	bool active = uk_vas_get_active() == p->vas;

	p->active = p->active && active;
	p->visible = p->visible && pt && pt->process == p->process &&
		pt->tid != p->process->pid && tid2pthread(pt->tid) == pt;
	p->remote = p->remote && uk_sched_current() == p->target;
	/* Report an incorrect binding without touching an unmapped address. */
	if (active) {
		p->active = p->active &&
			__atomic_load_n(value, __ATOMIC_RELAXED) ==
			PREPARE_VALUE;
		__atomic_store_n(value, PREPARE_VALUE + 1, __ATOMIC_RELAXED);
	}
	uk_thread_exit();
}

static void prepare_pthread_child_reaped(struct uk_thread *thread)
{
	struct prepare_pthread_run *p = thread->priv;

	uk_semaphore_up(&p->child_reaped);
}

static void prepare_pthread_parent(void *arg)
{
	struct prepare_pthread_run *p = arg;
	struct uk_sched *s = p->target;
	struct uk_thread *child;
	__u32 *value = (__u32 *)p->address;
	pid_t tid;
	int rc;

#if CONFIG_LIBPOSIX_PROCESS_FORK
	p->kernel_fork_rejected = prepare_kernel_fork_rejected();
	p->kernel_exec_rejected = prepare_kernel_exec_rejected();
#endif
	for (unsigned int i = 0; i < 32; i++) {
		__atomic_store_n(value, PREPARE_VALUE, __ATOMIC_RELAXED);
		child = uk_thread_create_fn1(s->a, prepare_pthread_child, p,
					     s->a_stack, 0, s->a_auxstack, 0,
					     s->a_uktls, false, "owned-pthread",
					     p, prepare_pthread_child_reaped);
		if (!child)
			break;
		rc = uk_posix_process_create_pthread(child);
		if (rc) {
			uk_thread_release(child);
			break;
		}
		tid = uk_thread_uktls_var(child, pthread_self)->tid;
		p->bound = p->bound && child->vas == p->vas &&
			child->vas_ref == p->vas &&
			uk_refcount_read(&p->vas->refs) == 4;
		rc = uk_sched_thread_add(p->target, child);
		if (rc) {
			uk_thread_release(child);
			break;
		}
		if (uk_semaphore_down_to(&p->child_reaped,
					 PREPARE_TIMEOUT) == __NSEC_MAX)
			UK_CRASH("Process pthread was not reaped\n");
		prepare_pthread_refs(p->vas, 3);
		p->balanced = p->balanced && !tid2pthread(tid) &&
			uk_list_is_singular(&p->process->threads);
		p->active = p->active &&
			__atomic_load_n(value, __ATOMIC_RELAXED) ==
			PREPARE_VALUE + 1;
		p->cycles++;
	}
	uk_semaphore_up(&p->done);
	uk_semaphore_down(&p->finish);
	uk_thread_exit();
}

static void prepare_pthread_parent_reaped(struct uk_thread *thread)
{
	struct prepare_pthread_run *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

UK_TESTCASE(process_prepare, owned_pthread_inherits_vas)
{
	struct uk_sched *local = uk_sched_current(), *target = local;
	struct prepare_pthread_run probe = {
		.bound = true, .active = true, .visible = true,
		.balanced = true, .remote = true
	};
	struct uk_vas *kernel = uk_vas_get_kernel(), *vas = NULL;
	struct uk_thread *parent;
	struct posix_thread *pt;
	unsigned int cpus = uk_sched_started_count();
	unsigned int users = kernel->pt->shared_users;
	__vaddr_t address = UK_PAGING_VADDR_ANY;
	pid_t pid;
	int rc;

	for (unsigned int i = 0; i < cpus && target == local; i++)
		target = uk_sched_select_round_robin(local);
	probe.target = target;
	uk_semaphore_init(&probe.child_reaped, 0);
	uk_semaphore_init(&probe.done, 0);
	uk_semaphore_init(&probe.finish, 0);
	uk_semaphore_init(&probe.reaped, 0);
	parent = uk_thread_create_fn1(local->a, prepare_pthread_parent, &probe,
				      local->a_stack, 0, local->a_auxstack, 0,
				      local->a_uktls, false, "pthread-parent",
				      &probe, prepare_pthread_parent_reaped);
	UK_TEST_EXPECT_NOT_NULL(parent);
	if (!parent)
		return;
	uk_thread_set_blocked(parent);
	pid = uk_posix_process_prepare_spawn(local->a, parent, &vas);
	UK_TEST_EXPECT(pid > 0);
	if (pid <= 0)
		goto release_parent;
	pt = uk_thread_uktls_var(parent, pthread_self);
	probe.process = pt->process;
	probe.vas = vas;
	rc = uk_vma_map_anon(vas, &address, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort;
	probe.address = address;
	rc = uk_sched_thread_add(local, parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto abort;
	rc = uk_posix_process_publish_prepared(parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		UK_TEST_EXPECT_ZERO(uk_sched_thread_remove(parent));
		goto abort;
	}
	uk_thread_wake(parent);
	if (uk_semaphore_down_to(&probe.done, PREPARE_TIMEOUT) == __NSEC_MAX)
		UK_CRASH("Process pthread cycles made no progress\n");
	pprocess_release_pthread(pt);
	pprocess_release(probe.process);
	uk_semaphore_up(&probe.finish);
	if (uk_semaphore_down_to(&probe.reaped, PREPARE_TIMEOUT) == __NSEC_MAX)
		UK_CRASH("Process pthread parent was not reaped\n");
	prepare_pthread_refs(vas, 1);
	uk_vas_put(vas);
	UK_TEST_EXPECT_SNUM_EQ(probe.cycles, 32);
	UK_TEST_EXPECT(probe.bound);
	UK_TEST_EXPECT(probe.active);
	UK_TEST_EXPECT(probe.visible);
	UK_TEST_EXPECT(probe.balanced);
#if CONFIG_LIBPOSIX_PROCESS_FORK
	UK_TEST_EXPECT(probe.kernel_fork_rejected);
	uk_pr_info("PROCESS_FORK_REJECT_PASS private_callback=1\n");
	UK_TEST_EXPECT(probe.kernel_exec_rejected);
	uk_pr_info("PROCESS_EXEC_REJECT_PASS private_callback=1\n");
#endif
	UK_TEST_EXPECT(probe.remote);
	UK_TEST_EXPECT((target != local) == (cpus > 1));
	UK_TEST_EXPECT_NULL(pid2pprocess(pid));
	UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
	uk_pr_info("PROCESS_PTHREAD_VAS_PASS cycles=%u remote=%u\n",
		   probe.cycles, target != local);
	return;
abort:
	UK_TEST_EXPECT_ZERO(uk_posix_process_abort_prepared(parent));
release_parent:
	uk_thread_release(parent);
	uk_vas_put(vas);
}

#endif /* CONFIG_LIBPOSIX_FDTAB_MULTITAB && !CONFIG_LIBVFSCORE */

_UK_TESTSUITE(process_prepare, NULL);

static void __constructor run_process_prepare(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_prepare));
}
