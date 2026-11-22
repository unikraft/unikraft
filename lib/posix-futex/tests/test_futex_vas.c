/* SPDX-License-Identifier: BSD-3-Clause */

#include <asm/unistd.h>
#include <errno.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <uk/crash.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/plat/time.h>
#include <uk/print.h>
#include <uk/process.h>
#include <uk/refcount.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/syscall.h>
#include <uk/test.h>
#include <uk/vmem/process.h>
#include <uk/vmem/vma_types.h>

#define FUTEX_VAS_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)
#define FUTEX_VAS_WORD UK_VAS_PROCESS_BASE
#define FUTEX_VAS_TARGET (FUTEX_VAS_WORD + UK_PAGING_PAGE_SIZE)

struct futex_test_spaces {
	struct uk_vas *parent;
	struct uk_vas *child;
};

struct futex_test_probe {
	struct uk_thread *thread;
	struct uk_vas *vas;
	struct uk_semaphore finish;
	struct uk_semaphore reaped;
	__vaddr_t addr;
	__vaddr_t clear_addr;
	__u32 expected;
	__u32 bitset;
	__u32 observed;
	int cpu;
	int op;
	long result;
	long tid;
	bool active;
	bool ready;
	bool completed;
};

static long futex_test_call(__vaddr_t addr, int op, __u32 val,
			    const struct timespec *timeout,
			    __vaddr_t addr2, __u32 val3)
{
	return uk_syscall_r_static(SYS_futex, (long)addr, (long)op,
				   (long)val, (long)timeout,
				   (long)addr2, (long)val3);
}

static long futex_test_in_vas(struct uk_vas *vas, __vaddr_t addr, int op,
			      __u32 val, __vaddr_t addr2, __u32 val3)
{
	struct uk_thread *current = uk_thread_current();
	struct uk_vas *saved = current->vas;
	long result;
	int rc;

	rc = uk_thread_set_vas(current, vas);
	if (rc)
		return rc;
	result = futex_test_call(addr, op, val, NULL, addr2, val3);
	if (uk_thread_set_vas(current, saved))
		UK_CRASH("Cannot restore futex test address space\n");
	return result;
}

static int futex_test_spaces_new(struct futex_test_spaces *s, bool shared,
				 __u32 initial)
{
	__vaddr_t addr = FUTEX_VAS_WORD;
	int rc;

	memset_isr(s, 0, sizeof(*s));
	rc = uk_vas_create_process(uk_vas_get_kernel()->a, &s->parent);
	if (rc)
		return rc;
	if (shared) {
		/* Keep adjacent backing objects distinct. */
		rc = uk_vma_map(s->parent, &addr, UK_PAGING_PAGE_SIZE,
				UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL,
				&uk_vma_shared_anon_ops, NULL);
		if (!rc) {
			addr = FUTEX_VAS_TARGET;
			rc = uk_vma_map(s->parent, &addr, UK_PAGING_PAGE_SIZE,
					UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL,
					&uk_vma_shared_anon_ops, NULL);
		}
	} else {
		rc = uk_vma_map_anon(s->parent, &addr, 2 * UK_PAGING_PAGE_SIZE,
				     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	}
	if (!rc && initial)
		rc = uk_vas_store_u32(s->parent, FUTEX_VAS_WORD, initial);
	if (!rc)
		rc = uk_vas_clone_process(s->parent, &s->child);
	return rc;
}

static void futex_test_spaces_free(struct futex_test_spaces *s)
{
	__nsec end = ukplat_monotonic_clock() + FUTEX_VAS_TIMEOUT;

	while ((s->parent && uk_refcount_read(&s->parent->refs) != 1) ||
	       (s->child && uk_refcount_read(&s->child->refs) != 1)) {
		if (ukplat_monotonic_clock() >= end)
			UK_CRASH("Futex test retained an address-space pin\n");
		uk_arch_spinwait();
	}
	uk_vas_put(s->child);
	uk_vas_put(s->parent);
}

static void futex_test_probe_fn(void *arg) __noreturn;

static void futex_test_probe_fn(void *arg)
{
	struct futex_test_probe *p = arg;
	struct timespec timeout = { .tv_sec = 5 };
	__nsec end;

	p->active = uk_vas_get_active() == p->vas &&
		uk_paging_pt_get_active() == p->vas->pt &&
		uk_sched_current()->lcpu_idx == p->cpu;
	if (p->clear_addr)
		p->tid = uk_syscall_r_static(SYS_set_tid_address,
					     (long)p->clear_addr);
	__atomic_store_n(&p->ready, true, __ATOMIC_RELEASE);
	if (!p->clear_addr) {
		if ((p->op & FUTEX_CMD_MASK) == FUTEX_WAIT_BITSET) {
			end = ukplat_monotonic_clock() + FUTEX_VAS_TIMEOUT;
			timeout.tv_sec = end / UKARCH_NSEC_PER_SEC;
			timeout.tv_nsec = end % UKARCH_NSEC_PER_SEC;
		}
		p->result = p->active ? futex_test_call(p->addr, p->op,
				p->expected, &timeout, 0, p->bitset) : -EFAULT;
		if (!p->result)
			p->observed = __atomic_load_n((__u32 *)p->addr,
						      __ATOMIC_ACQUIRE);
		__atomic_store_n(&p->completed, true, __ATOMIC_RELEASE);
	}
	/* Keep the thread object alive until the controller finishes checks. */
	uk_semaphore_down(&p->finish);
	uk_thread_exit();
}

static void futex_test_probe_reaped(struct uk_thread *thread)
{
	struct futex_test_probe *p = thread->priv;

	uk_semaphore_up(&p->reaped);
}

static int futex_test_probe_start(struct futex_test_probe *p)
{
	struct uk_sched *local = uk_sched_current(), *sched = local;
	unsigned int i, cpus = uk_sched_started_count();
	int rc;

	for (i = 0; i < cpus; i++) {
		sched = uk_sched_select_round_robin(local);
		if (sched->lcpu_idx != local->lcpu_idx || cpus == 1)
			break;
	}
	p->cpu = sched->lcpu_idx;
	uk_semaphore_init(&p->finish, 0);
	uk_semaphore_init(&p->reaped, 0);
	p->thread = uk_thread_create_fn1(sched->a, futex_test_probe_fn, p,
					 sched->a_stack, 0,
					 sched->a_auxstack, 0,
					 sched->a_uktls, false,
					 "futex-vas", p,
					 futex_test_probe_reaped);
	if (!p->thread)
		return -ENOMEM;
	rc = uk_posix_process_create_pthread(p->thread);
	if (!rc)
		rc = uk_thread_set_vas(p->thread, p->vas);
	if (!rc)
		rc = uk_sched_thread_add(sched, p->thread);
	if (rc) {
		uk_thread_release(p->thread);
		p->thread = NULL;
	}
	return rc;
}

static int futex_test_probe_wait(struct futex_test_probe *p, bool completed)
{
	__nsec end = ukplat_monotonic_clock() + FUTEX_VAS_TIMEOUT;

	for (;;) {
		if (__atomic_load_n(&p->ready, __ATOMIC_ACQUIRE)) {
			if (__atomic_load_n(&p->completed, __ATOMIC_ACQUIRE))
				return completed ? 0 : -EIO;
			if (!completed && uk_thread_is_blocked(p->thread))
				return 0;
		}
		if (ukplat_monotonic_clock() >= end)
			return -ETIMEDOUT;
		uk_sched_yield();
	}
}

static void futex_test_probe_stop(struct futex_test_probe *p)
{
	if (!p->thread)
		return;
	if (__atomic_load_n(&p->completed, __ATOMIC_ACQUIRE))
		uk_semaphore_up(&p->finish);
	else
		uk_sched_thread_terminate(p->thread);
	if (uk_semaphore_down_to(&p->reaped, FUTEX_VAS_TIMEOUT) == __NSEC_MAX)
		UK_CRASH("Futex test thread was not reaped\n");
	p->thread = NULL;
}

UK_TESTCASE(futex_vas, private_same_address_isolated)
{
	struct futex_test_spaces s;
	struct futex_test_probe p;
	unsigned int mode, remote = 0;
	long woke;
	bool finished;
	int rc = futex_test_spaces_new(&s, false, 7);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	for (mode = 0; mode < 2; mode++) {
		int flags = mode ? FUTEX_PRIVATE_FLAG : 0;

		memset_isr(&p, 0, sizeof(p));
		p.vas = s.child;
		p.addr = FUTEX_VAS_WORD;
		p.op = FUTEX_WAIT | flags;
		p.expected = 7;
		rc = futex_test_probe_start(&p);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
		rc = futex_test_probe_wait(&p, false);
		UK_TEST_EXPECT_ZERO(rc);
		if (!rc) {
			woke = futex_test_in_vas(s.parent, FUTEX_VAS_WORD,
						 FUTEX_WAKE | flags, 1, 0, 0);
			UK_TEST_EXPECT_SNUM_EQ(woke, 0);
			finished = __atomic_load_n(&p.completed,
						   __ATOMIC_ACQUIRE);
			UK_TEST_EXPECT(!finished);
			woke = futex_test_in_vas(s.child, FUTEX_VAS_WORD,
						 FUTEX_WAKE | flags, 1, 0, 0);
			UK_TEST_EXPECT_SNUM_EQ(woke, 1);
			UK_TEST_EXPECT_ZERO(futex_test_probe_wait(&p, true));
			UK_TEST_EXPECT_ZERO(p.result);
			UK_TEST_EXPECT(p.active);
			remote = p.cpu != uk_sched_current()->lcpu_idx;
		}
		futex_test_probe_stop(&p);
		if (rc)
			goto out;
	}
	uk_pr_info("FUTEX_VAS_PASS private isolated=2 remote=%u\n", remote);
out:
	futex_test_spaces_free(&s);
}

UK_TESTCASE(futex_vas, shared_backing_bitset)
{
	struct futex_test_spaces s;
	struct futex_test_probe p = { 0 };
	long woke;
	int rc = futex_test_spaces_new(&s, true, 0);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	p.vas = s.child;
	p.addr = FUTEX_VAS_WORD;
	p.op = FUTEX_WAIT_BITSET;
	p.bitset = 2;
	rc = futex_test_probe_start(&p);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = futex_test_probe_wait(&p, false);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto stop;
	woke = futex_test_in_vas(s.parent, FUTEX_VAS_WORD,
				 FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, 0, 0);
	UK_TEST_EXPECT_SNUM_EQ(woke, 0);
	woke = futex_test_in_vas(s.parent, FUTEX_VAS_WORD,
				 FUTEX_WAKE_BITSET, 1, 0, 1);
	UK_TEST_EXPECT_SNUM_EQ(woke, 0);
	UK_TEST_EXPECT_ZERO(uk_vas_store_u32(s.parent, FUTEX_VAS_WORD, 1));
	woke = futex_test_in_vas(s.parent, FUTEX_VAS_WORD,
				 FUTEX_WAKE_BITSET, 1, 0, 2);
	UK_TEST_EXPECT_SNUM_EQ(woke, 1);
	UK_TEST_EXPECT_ZERO(futex_test_probe_wait(&p, true));
	UK_TEST_EXPECT_ZERO(p.result);
	UK_TEST_EXPECT_SNUM_EQ(p.observed, 1);
	UK_TEST_EXPECT(p.active);
	uk_pr_info("FUTEX_VAS_PASS shared bitset=1 private_miss=1 remote=%u\n",
		   p.cpu != uk_sched_current()->lcpu_idx);
stop:
	futex_test_probe_stop(&p);
out:
	futex_test_spaces_free(&s);
}

_UK_TESTSUITE(futex_vas, NULL);

static void __constructor run_futex_vas(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(futex_vas));
}
