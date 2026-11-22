/* SPDX-License-Identifier: BSD-3-Clause */

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <uk/crash.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/print.h>
#include <uk/sched.h>
#include <uk/test.h>
#include <uk/vmem/process.h>

struct mmap_cow_state {
	struct uk_thread *current;
	struct uk_vas *saved;
	struct uk_vas *parent;
	struct uk_vas *child;
};

static int mmap_cow_begin(struct mmap_cow_state *s)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	int rc;

	memset_isr(s, 0, sizeof(*s));
	s->current = uk_thread_current();
	s->saved = s->current->vas;
	/* The test's control stack and TLS must stay in the shared kernel. */
	if (uk_vas_get_active() != kernel || (s->saved && s->saved != kernel))
		return -EBUSY;
	rc = uk_vas_create_process(kernel->a, &s->parent);
	if (rc)
		return rc;
	rc = uk_thread_set_vas(s->current, s->parent);
	if (rc) {
		uk_vas_put(s->parent);
		s->parent = NULL;
	}
	return rc;
}

static void mmap_cow_end(struct mmap_cow_state *s)
{
	if (uk_thread_set_vas(s->current, s->saved))
		UK_CRASH("Cannot restore POSIX mmap test address space\n");
	uk_vas_put(s->child);
	uk_vas_put(s->parent);
}

UK_TESTCASE(posix_mmap_cow, private_mapping_clone_and_parent_exit)
{
	struct mmap_cow_state s;
	__u32 *first, *second;
	int rc = mmap_cow_begin(&s);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	first = mmap(NULL, 2 * UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(first != MAP_FAILED);
	if (first == MAP_FAILED)
		goto out;
	second = (void *)((__uptr)first + UK_PAGING_PAGE_SIZE);
	__atomic_store_n(first, 0x1111U, __ATOMIC_RELAXED);
	__atomic_store_n(second, 0x2222U, __ATOMIC_RELAXED);
	rc = uk_vas_clone_process(s.parent, &s.child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_thread_set_vas(s.current, s.child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(first, __ATOMIC_RELAXED),
			       0x1111);
	__atomic_store_n(first, 0x3333U, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.parent));
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(first, __ATOMIC_RELAXED),
			       0x1111);
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.child));
	uk_vas_put(s.parent);
	s.parent = NULL;
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(first, __ATOMIC_RELAXED),
			       0x3333);
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(second, __ATOMIC_RELAXED),
			       0x2222);
	UK_TEST_EXPECT_ZERO(munmap(first, 2 * UK_PAGING_PAGE_SIZE));
	uk_pr_info("POSIX_MMAP_COW_PASS private\n");
out:
	mmap_cow_end(&s);
}

UK_TESTCASE(posix_mmap_cow, lazy_protection_stays_private)
{
	struct mmap_cow_state s;
	__u32 *value;
	int rc = mmap_cow_begin(&s);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	value = mmap(NULL, UK_PAGING_PAGE_SIZE, PROT_NONE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(value != MAP_FAILED);
	if (value == MAP_FAILED)
		goto out;
	rc = uk_vas_clone_process(s.parent, &s.child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.child));
	rc = mprotect(value, UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	__atomic_store_n(value, 0x4444U, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.parent));
	rc = mprotect(value, UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(__atomic_load_n(value, __ATOMIC_RELAXED));
	UK_TEST_EXPECT_ZERO(munmap(value, UK_PAGING_PAGE_SIZE));
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.child));
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(value, __ATOMIC_RELAXED),
			       0x4444);
	UK_TEST_EXPECT_ZERO(munmap(value, UK_PAGING_PAGE_SIZE));
	uk_pr_info("POSIX_MMAP_COW_PASS protection\n");
out:
	mmap_cow_end(&s);
}

UK_TESTCASE(posix_mmap_cow, shared_mapping_split_and_parent_exit)
{
	struct mmap_cow_state s;
	__u32 *base, *middle, *last;
	int rc = mmap_cow_begin(&s);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	base = mmap(NULL, 3 * UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE,
		    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(base != MAP_FAILED);
	if (base == MAP_FAILED)
		goto out;
	middle = (void *)((__uptr)base + UK_PAGING_PAGE_SIZE);
	last = (void *)((__uptr)middle + UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT_ZERO(__atomic_load_n(middle, __ATOMIC_RELAXED));
	__atomic_store_n(base, 0x1111U, __ATOMIC_RELAXED);
	__atomic_store_n(middle, 0x2222U, __ATOMIC_RELAXED);
	__atomic_store_n(last, 0x3333U, __ATOMIC_RELAXED);
	/* Clone an already split VMA, including its nonzero backing offset. */
	rc = mprotect(middle, UK_PAGING_PAGE_SIZE, PROT_READ);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_clone_process(s.parent, &s.child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.child));
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(middle, __ATOMIC_RELAXED),
			       0x2222);
	__atomic_store_n(base, 0x4444U, __ATOMIC_RELAXED);
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.parent));
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(base, __ATOMIC_RELAXED),
			       0x4444);
	/* Removing the parent's middle alias must retain the child's page. */
	UK_TEST_EXPECT_ZERO(munmap(middle, UK_PAGING_PAGE_SIZE));
	UK_TEST_EXPECT_ZERO(uk_thread_set_vas(s.current, s.child));
	rc = mprotect(middle, UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = madvise(middle, UK_PAGING_PAGE_SIZE, MADV_DONTNEED);
	UK_TEST_EXPECT_ZERO(rc);
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(middle, __ATOMIC_RELAXED),
			       0x2222);
	__atomic_store_n(middle, 0x5555U, __ATOMIC_RELAXED);
	uk_vas_put(s.parent);
	s.parent = NULL;
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(base, __ATOMIC_RELAXED),
			       0x4444);
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(middle, __ATOMIC_RELAXED),
			       0x5555);
	UK_TEST_EXPECT_SNUM_EQ(__atomic_load_n(last, __ATOMIC_RELAXED),
			       0x3333);
	UK_TEST_EXPECT_ZERO(munmap(base, 3 * UK_PAGING_PAGE_SIZE));
	uk_pr_info("POSIX_MMAP_COW_PASS shared_split\n");
out:
	mmap_cow_end(&s);
}

static int mmap_shared_cycle(unsigned int cycle)
{
	struct mmap_cow_state s;
	__u32 *value;
	int rc = mmap_cow_begin(&s);

	if (rc)
		return rc;
	value = mmap(NULL, UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE,
		     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (value == MAP_FAILED) {
		rc = -errno;
		goto out;
	}
	/* Both first faults must share backing, without a PTE at clone time. */
	rc = uk_vas_clone_process(s.parent, &s.child);
	if (rc)
		goto out;
	__atomic_store_n(value, cycle, __ATOMIC_RELAXED);
	rc = uk_thread_set_vas(s.current, s.child);
	if (rc)
		goto out;
	if (__atomic_load_n(value, __ATOMIC_RELAXED) != cycle) {
		rc = -EIO;
		goto out;
	}
	__atomic_store_n(value, cycle + 64, __ATOMIC_RELAXED);
	rc = uk_thread_set_vas(s.current, s.parent);
	if (rc)
		goto out;
	if (__atomic_load_n(value, __ATOMIC_RELAXED) != cycle + 64)
		rc = -EIO;
out:
	mmap_cow_end(&s);
	return rc;
}

UK_TESTCASE(posix_mmap_cow, shared_mapping_resource_balance)
{
	struct uk_pagetable *pt = uk_vas_get_kernel()->pt;
	unsigned long first_free = 0;
	unsigned int users = pt->shared_users;
	unsigned int cycle;
	int rc;

	for (cycle = 1; cycle <= 64; cycle++) {
		rc = mmap_shared_cycle(cycle);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(pt->shared_users, users);
		if (cycle == 1)
			first_free = pt->fa->free_memory;
		UK_TEST_EXPECT_SNUM_EQ(pt->fa->free_memory, first_free);
		if (rc)
			return;
	}
	uk_pr_info("POSIX_MMAP_COW_PASS shared_balance cycles=64\n");
}

_UK_TESTSUITE(posix_mmap_cow, NULL);

static void __constructor run_posix_mmap_cow(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(posix_mmap_cow));
}
