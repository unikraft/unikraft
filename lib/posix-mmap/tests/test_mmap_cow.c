/* SPDX-License-Identifier: BSD-3-Clause */

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <uk/crash.h>
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

UK_TESTCASE(posix_mmap_cow, shared_mapping_requires_shared_backing)
{
	struct mmap_cow_state s;
	void *addr;
	int rc = mmap_cow_begin(&s);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	errno = 0;
	addr = mmap(NULL, UK_PAGING_PAGE_SIZE, PROT_READ | PROT_WRITE,
		    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	UK_TEST_EXPECT(addr == MAP_FAILED);
	UK_TEST_EXPECT_SNUM_EQ(errno, ENOTSUP);
	UK_TEST_EXPECT(uk_list_empty(&s.parent->vma_list));
	rc = uk_vas_clone_process(s.parent, &s.child);
	UK_TEST_EXPECT_ZERO(rc);
	uk_pr_info("POSIX_MMAP_COW_PASS shared_rejected\n");
	mmap_cow_end(&s);
}

_UK_TESTSUITE(posix_mmap_cow, NULL);

static void __constructor run_posix_mmap_cow(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(posix_mmap_cow));
}
