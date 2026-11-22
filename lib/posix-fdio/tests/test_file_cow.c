/* SPDX-License-Identifier: BSD-3-Clause */

#define _GNU_SOURCE

#include <asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <time.h>
#include <uk/alloc.h>
#include <uk/alloc_impl.h>
#include <uk/crash.h>
#include <uk/falloc.h>
#include <uk/file.h>
#include <uk/file/nops.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/plat/time.h>
#include <uk/posix-fdtab.h>
#include <uk/print.h>
#include <uk/process.h>
#include <uk/refcount.h>
#include <uk/sched.h>
#include <uk/semaphore.h>
#include <uk/syscall.h>
#include <uk/test.h>
#include <uk/vmem/process.h>

#define FILE_COW_ADDR UK_VAS_PROCESS_BASE
#define FILE_COW_ALIAS (FILE_COW_ADDR + 8 * UK_PAGING_PAGE_SIZE)
#define FILE_COW_TIMEOUT (5 * UKARCH_NSEC_PER_SEC)
#define FILE_COW_INITIAL 0x11110000U

/* Separate file objects and OFDs refer to one stable resident test node. */
struct file_cow_fixture {
	struct uk_file files[2];
	uk_file_refcnt refs[2];
	struct uk_file_state state;
	unsigned char *data;
	size_t size;
	size_t capacity;
	size_t pinned;
	unsigned long pages;
	unsigned int reads;
	unsigned int bad_context;
	unsigned int released[2];
	bool owned[2];
	int fd[2];
};

static const char file_cow_volume;

static void file_cow_context(struct file_cow_fixture *f)
{
	if (uk_lcpu_irqs_disabled())
		__atomic_fetch_add(&f->bad_context, 1, __ATOMIC_RELAXED);
}

static ssize_t file_cow_read(const struct uk_file *file,
			     const struct iovec *iov, size_t count,
			     size_t off, long flags __unused)
{
	struct file_cow_fixture *f = file->node;
	size_t i, len, done = 0;

	file_cow_context(f);
	__atomic_fetch_add(&f->reads, 1, __ATOMIC_RELAXED);
	for (i = 0; i < count && off < f->size; i++) {
		len = MIN(iov[i].iov_len, f->size - off);
		memcpy_isr(iov[i].iov_base, f->data + off, len);
		off += len;
		done += len;
	}
	return done;
}

static ssize_t file_cow_mem(const struct uk_file *file, enum uk_file_mem_op op,
			    size_t off, size_t len,
			    struct iovec *iov, size_t count)
{
	struct file_cow_fixture *f = file->node;

	file_cow_context(f);
	if (off > f->capacity || len > f->capacity - off ||
	    !UK_PAGING_PAGE_ALIGNED(off) || !UK_PAGING_PAGE_ALIGNED(len))
		return -EINVAL;
	switch (op) {
	case UKFILE_MEM_ACQUIRE_PINNED:
		f->pinned += len;
		return len;
	case UKFILE_MEM_RELEASE:
		UK_ASSERT(f->pinned >= len);
		f->pinned -= len;
		return 0;
	case UKFILE_MEM_RETRIEVE:
		if (!count || !f->pinned)
			return -EINVAL;
		iov[0].iov_base = f->data + off;
		iov[0].iov_len = len;
		return 1;
	default:
		return -ENOTSUP;
	}
}

static int file_cow_ctl(const struct uk_file *file, int family, int request,
			uintptr_t arg1 __unused, uintptr_t arg2 __unused,
			uintptr_t arg3 __unused)
{
	file_cow_context(file->node);
	return family == UKFILE_CTL_FILE && request == UKFILE_CTL_FILE_SYNC ?
		0 : -EOPNOTSUPP;
}

static void file_cow_release(const struct uk_file *file, int what)
{
	struct file_cow_fixture *f = file->node;
	unsigned int i = file == &f->files[1];

	file_cow_context(f);
	__atomic_fetch_or(&f->released[i], what, __ATOMIC_RELEASE);
}

static const struct uk_file_ops file_cow_ops = {
	.read = file_cow_read,
	.write = uk_file_nop_write,
	.mem = file_cow_mem,
	.getstat = uk_file_nop_getstat,
	.setstat = uk_file_nop_setstat,
	.ctl = file_cow_ctl
};

static int file_cow_fixture_init_pages(struct file_cow_fixture *f,
				       unsigned long pages)
{
	unsigned int i;

	memset_isr(f, 0, sizeof(*f));
	f->fd[0] = -1;
	f->fd[1] = -1;
	f->pages = pages;
	f->capacity = f->pages * UK_PAGING_PAGE_SIZE;
	f->size = f->capacity - UK_PAGING_PAGE_SIZE / 2;
	f->data = uk_palloc(uk_vas_get_kernel()->a, f->pages);
	if (!f->data)
		return -ENOMEM;
	memset_isr(f->data, 0, f->capacity);
	for (i = 0; i < f->pages; i++)
		*(__u32 *)(f->data + i * UK_PAGING_PAGE_SIZE) =
			FILE_COW_INITIAL + i;
	f->state = UK_FILE_STATE_INIT_VALUE(f->state);
	for (i = 0; i < 2; i++) {
		f->refs[i] = UK_FILE_REFCNT_INIT_VALUE(f->refs[i]);
		f->files[i] = (struct uk_file) {
			.vol = &file_cow_volume,
			.node = f,
			.ops = &file_cow_ops,
			.refcnt = &f->refs[i],
			.state = &f->state,
			._release = file_cow_release
		};
		f->owned[i] = true;
		f->fd[i] = uk_fdtab_open(&f->files[i], O_RDWR);
		if (f->fd[i] < 0)
			return f->fd[i];
	}
	return 0;
}

static int file_cow_fixture_init(struct file_cow_fixture *f)
{
	return file_cow_fixture_init_pages(f, 3);
}

static void file_cow_fixture_close(struct file_cow_fixture *f)
{
	unsigned int i;

	for (i = 0; i < 2; i++) {
		if (f->fd[i] >= 0) {
			if (uk_sys_close(f->fd[i]))
				UK_CRASH("Cannot close file COW test handle\n");
			f->fd[i] = -1;
		}
		if (f->owned[i]) {
			uk_file_release(&f->files[i]);
			f->owned[i] = false;
		}
	}
}

static int file_cow_fixture_free(struct file_cow_fixture *f)
{
	const unsigned int all = UK_FILE_RELEASE_RES | UK_FILE_RELEASE_OBJ;
	unsigned int i;

	file_cow_fixture_close(f);

	for (i = 0; i < 2; i++) {
		/* A live identity must not outlast this fixture. */
		if (f->files[i].ops &&
		    __atomic_load_n(&f->released[i], __ATOMIC_ACQUIRE) != all)
			UK_CRASH("File COW test retained a file reference\n");
	}
	if (f->pinned)
		UK_CRASH("File COW test retained backing pages\n");
	if (f->data)
		uk_pfree(uk_vas_get_kernel()->a, f->data, f->pages);
	return f->bad_context ? -EWOULDBLOCK : 0;
}

static void file_cow_bind(struct uk_vas *vas)
{
	if (uk_thread_set_vas(uk_thread_current(), vas))
		UK_CRASH("Cannot switch file COW test address space\n");
}

static void *file_cow_map(struct uk_vas *vas, __vaddr_t addr, size_t len,
			  int fd, bool shared)
{
	struct uk_vas *saved = uk_thread_current()->vas;
	void *result;

	file_cow_bind(vas);
	result = mmap((void *)addr, len, PROT_READ | PROT_WRITE,
		      MAP_FIXED | (shared ? MAP_SHARED : MAP_PRIVATE), fd, 0);
	file_cow_bind(saved);
	return result;
}

UK_TESTCASE(file_cow, private_clone_split_and_advice)
{
	struct file_cow_fixture f;
	struct uk_vas *saved = uk_thread_current()->vas;
	struct uk_vas *parent = NULL, *child = NULL;
	const struct uk_vma *merged, *tail;
	__u32 *first = (void *)FILE_COW_ADDR;
	__u32 *last = (void *)(FILE_COW_ADDR + 2 * UK_PAGING_PAGE_SIZE);
	unsigned int reads;
	int rc = file_cow_fixture_init(&f);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_create_process(uk_vas_get_kernel()->a, &parent);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	if (file_cow_map(parent, FILE_COW_ADDR, f.capacity, f.fd[0], false)
	    == MAP_FAILED) {
		UK_TEST_EXPECT(false);
		goto out;
	}
	reads = f.reads;
	file_cow_bind(parent);
	UK_TEST_EXPECT_SNUM_EQ(*first, FILE_COW_INITIAL);
	*first = 0x3333U;
	rc = uk_vas_clone_process(parent, &child);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	file_cow_bind(child);
	UK_TEST_EXPECT_SNUM_EQ(*first, 0x3333);
	*first = 0x4444U;
	file_cow_bind(parent);
	UK_TEST_EXPECT_SNUM_EQ(*first, 0x3333);
	file_cow_bind(child);
	UK_TEST_EXPECT_ZERO(mprotect((void *)(FILE_COW_ADDR +
		UK_PAGING_PAGE_SIZE), UK_PAGING_PAGE_SIZE, PROT_READ));
	UK_TEST_EXPECT_SNUM_EQ(*last, FILE_COW_INITIAL + 2);
	UK_TEST_EXPECT_ZERO(*(last + UK_PAGING_PAGE_SIZE / sizeof(*last) - 1));
	UK_TEST_EXPECT_ZERO(mprotect((void *)(FILE_COW_ADDR +
		UK_PAGING_PAGE_SIZE), UK_PAGING_PAGE_SIZE,
		PROT_READ | PROT_WRITE));
	merged = uk_vma_find(child, FILE_COW_ADDR);
	tail = uk_vma_find(child, FILE_COW_ADDR + 2 * UK_PAGING_PAGE_SIZE);
	UK_TEST_EXPECT(merged && merged == tail);
	UK_TEST_EXPECT_ZERO(mprotect((void *)(FILE_COW_ADDR +
		UK_PAGING_PAGE_SIZE), UK_PAGING_PAGE_SIZE, PROT_READ));
	UK_TEST_EXPECT_ZERO(munmap((void *)(FILE_COW_ADDR +
		UK_PAGING_PAGE_SIZE), UK_PAGING_PAGE_SIZE));
	UK_TEST_EXPECT_ZERO(madvise(first, UK_PAGING_PAGE_SIZE, MADV_DONTNEED));
	UK_TEST_EXPECT_SNUM_EQ(*first, FILE_COW_INITIAL);
	file_cow_bind(parent);
	UK_TEST_EXPECT_SNUM_EQ(*first, 0x3333);
	file_cow_bind(child);
	uk_vas_put(parent);
	parent = NULL;
	*last = 0x5555U;
	UK_TEST_EXPECT_SNUM_EQ(*last, 0x5555);
	UK_TEST_EXPECT_ZERO(madvise(last, UK_PAGING_PAGE_SIZE, MADV_DONTNEED));
	UK_TEST_EXPECT_SNUM_EQ(*last, FILE_COW_INITIAL + 2);
	UK_TEST_EXPECT_SNUM_EQ(f.reads, reads);
	uk_pr_info("FILE_COW_PASS private_clone_split_merge_advice\n");
out:
	file_cow_bind(saved);
	uk_vas_put(child);
	uk_vas_put(parent);
	UK_TEST_EXPECT_ZERO(file_cow_fixture_free(&f));
}

struct file_cow_waiter {
	struct uk_thread *thread;
	struct uk_vas *vas;
	struct uk_semaphore finish;
	struct uk_semaphore reaped;
	__vaddr_t addr;
	int cpu;
	long result;
	__u32 observed;
	bool active;
	bool ready;
	bool done;
};

static long file_cow_futex(__vaddr_t addr, int op, __u32 value,
			   const struct timespec *timeout)
{
	return uk_syscall_r_static(SYS_futex, (long)addr, (long)op,
				   (long)value, (long)timeout, 0L, 0L);
}

static void file_cow_wait(void *arg) __noreturn;

static void file_cow_wait(void *arg)
{
	struct file_cow_waiter *w = arg;
	struct timespec timeout = { .tv_sec = 5 };

	w->active = uk_vas_get_active() == w->vas &&
		uk_paging_pt_get_active() == w->vas->pt &&
		uk_sched_current()->lcpu_idx == w->cpu;
	__atomic_store_n(&w->ready, true, __ATOMIC_RELEASE);
	w->result = w->active ? file_cow_futex(w->addr, FUTEX_WAIT, 0,
					      &timeout) : -EFAULT;
	if (!w->result)
		w->observed = __atomic_load_n((__u32 *)w->addr,
					      __ATOMIC_ACQUIRE);
	__atomic_store_n(&w->done, true, __ATOMIC_RELEASE);
	uk_semaphore_down(&w->finish);
	uk_thread_exit();
}

static void file_cow_reaped(struct uk_thread *thread)
{
	struct file_cow_waiter *w = thread->priv;

	uk_semaphore_up(&w->reaped);
}

static int file_cow_waiter_start(struct file_cow_waiter *w)
{
	struct uk_sched *local = uk_sched_current(), *sched = local;
	unsigned int i, cpus = uk_sched_started_count();
	int rc;

	for (i = 0; i < cpus; i++) {
		sched = uk_sched_select_round_robin(local);
		if (sched->lcpu_idx != local->lcpu_idx || cpus == 1)
			break;
	}
	w->cpu = sched->lcpu_idx;
	uk_semaphore_init(&w->finish, 0);
	uk_semaphore_init(&w->reaped, 0);
	w->thread = uk_thread_create_fn1(sched->a, file_cow_wait, w,
					 sched->a_stack, 0, sched->a_auxstack,
					 0, sched->a_uktls, false, "file-cow",
					 w, file_cow_reaped);
	if (!w->thread)
		return -ENOMEM;
	rc = uk_posix_process_create_pthread(w->thread);
	if (!rc)
		rc = uk_thread_set_vas(w->thread, w->vas);
	if (!rc)
		rc = uk_sched_thread_add(sched, w->thread);
	if (rc) {
		uk_thread_release(w->thread);
		w->thread = NULL;
	}
	return rc;
}

static int file_cow_waiter_poll(struct file_cow_waiter *w, bool done)
{
	__nsec end = ukplat_monotonic_clock() + FILE_COW_TIMEOUT;

	for (;;) {
		if (__atomic_load_n(&w->ready, __ATOMIC_ACQUIRE)) {
			if (__atomic_load_n(&w->done, __ATOMIC_ACQUIRE))
				return done ? 0 : -EIO;
			if (!done && uk_thread_is_blocked(w->thread))
				return 0;
		}
		if (ukplat_monotonic_clock() >= end)
			return -ETIMEDOUT;
		uk_sched_yield();
	}
}

static void file_cow_drain(struct uk_vas *vas)
{
	__nsec end = ukplat_monotonic_clock() + FILE_COW_TIMEOUT;

	while (vas && uk_refcount_read(&vas->refs) != 1) {
		if (ukplat_monotonic_clock() >= end)
			UK_CRASH("File COW test retained a VAS pin\n");
		uk_arch_spinwait();
	}
}

static void file_cow_waiter_stop(struct file_cow_waiter *w)
{
	if (!w->thread)
		return;
	if (__atomic_load_n(&w->done, __ATOMIC_ACQUIRE))
		uk_semaphore_up(&w->finish);
	else
		uk_sched_thread_terminate(w->thread);
	if (uk_semaphore_down_to(&w->reaped, FILE_COW_TIMEOUT) == __NSEC_MAX)
		UK_CRASH("File COW test waiter was not reaped\n");
	w->thread = NULL;
}

UK_TESTCASE(file_cow, shared_independent_handles_and_futex)
{
	struct file_cow_fixture f;
	struct file_cow_waiter w = { 0 };
	struct uk_vma_shared_ref refs[3] = { 0 }, copy = { 0 };
	struct uk_vas *saved = uk_thread_current()->vas;
	struct uk_vas *a = NULL, *b = NULL, *clone = NULL;
	unsigned int i;
	bool pinned = false;
	long woke;
	int rc = file_cow_fixture_init(&f);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	*(__u32 *)f.data = 0;
	rc = uk_vas_create_process(uk_vas_get_kernel()->a, &a);
	if (!rc)
		rc = uk_vas_create_process(uk_vas_get_kernel()->a, &b);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	if (file_cow_map(a, FILE_COW_ADDR, UK_PAGING_PAGE_SIZE,
			 f.fd[0], true) == MAP_FAILED ||
	    file_cow_map(b, FILE_COW_ALIAS, UK_PAGING_PAGE_SIZE,
			 f.fd[1], true) == MAP_FAILED) {
		UK_TEST_EXPECT(false);
		goto out;
	}
	rc = uk_vas_clone_process(a, &clone);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vas_get_shared_ref(a, FILE_COW_ADDR, sizeof(__u32), &refs[0]);
	if (!rc)
		rc = uk_vas_get_shared_ref(b, FILE_COW_ALIAS,
					   sizeof(__u32), &refs[1]);
	if (!rc)
		rc = uk_vas_get_shared_ref(clone, FILE_COW_ADDR + sizeof(__u32),
					   sizeof(__u32), &refs[2]);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT(refs[0].object == refs[1].object);
	UK_TEST_EXPECT(refs[0].object == refs[2].object);
	UK_TEST_EXPECT_SNUM_EQ(refs[0].offset, refs[1].offset);
	UK_TEST_EXPECT_SNUM_EQ(refs[2].offset, refs[0].offset + sizeof(__u32));
	copy = refs[0];
	uk_vma_shared_ref_get(&copy);
	w.vas = a;
	w.addr = FILE_COW_ADDR;
	rc = file_cow_waiter_start(&w);
	if (!rc)
		rc = file_cow_waiter_poll(&w, false);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	file_cow_bind(b);
	__atomic_store_n((__u32 *)FILE_COW_ALIAS, 42, __ATOMIC_RELEASE);
	woke = file_cow_futex(FILE_COW_ALIAS, FUTEX_WAKE, 1, NULL);
	UK_TEST_EXPECT_SNUM_EQ(woke, 1);
	file_cow_bind(saved);
	rc = file_cow_waiter_poll(&w, true);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT(w.active);
	UK_TEST_EXPECT_ZERO(w.result);
	UK_TEST_EXPECT_SNUM_EQ(w.observed, 42);
	file_cow_waiter_stop(&w);
	file_cow_bind(clone);
	UK_TEST_EXPECT_SNUM_EQ(*(__u32 *)FILE_COW_ADDR, 42);
	file_cow_bind(saved);
	uk_vas_put(clone);
	clone = NULL;
	uk_vas_put(b);
	b = NULL;
	file_cow_drain(a);
	uk_vas_put(a);
	a = NULL;
	file_cow_fixture_close(&f);
	/* Only identity pins keep the first handle's resources alive now. */
	UK_TEST_EXPECT_ZERO(f.pinned);
	UK_TEST_EXPECT_ZERO(f.released[0]);
	UK_TEST_EXPECT_SNUM_EQ(f.released[1],
			       UK_FILE_RELEASE_RES | UK_FILE_RELEASE_OBJ);
	pinned = true;
	uk_pr_info("FILE_COW_PASS shared_handles_futex remote=%u\n",
		   w.cpu != uk_sched_current()->lcpu_idx);
out:
	file_cow_bind(saved);
	file_cow_waiter_stop(&w);
	uk_vas_put(clone);
	uk_vas_put(b);
	file_cow_drain(a);
	uk_vas_put(a);
	for (i = 0; i < 3; i++)
		uk_vma_shared_ref_put(&refs[i]);
	if (pinned)
		UK_TEST_EXPECT_ZERO(f.released[0]);
	uk_vma_shared_ref_put(&copy);
	UK_TEST_EXPECT_ZERO(file_cow_fixture_free(&f));
}

static int file_cow_balance_cycle(void)
{
	struct file_cow_fixture f;
	struct uk_vas *saved = uk_thread_current()->vas;
	struct uk_vas *parent = NULL, *child = NULL;
	int rc = file_cow_fixture_init(&f);

	if (rc)
		goto out;
	rc = uk_vas_create_process(uk_vas_get_kernel()->a, &parent);
	if (rc)
		goto out;
	rc = -ENOMEM;
	if (file_cow_map(parent, FILE_COW_ADDR, f.capacity, f.fd[0], false)
	    == MAP_FAILED ||
	    file_cow_map(parent, FILE_COW_ALIAS, UK_PAGING_PAGE_SIZE,
			 f.fd[1], true) == MAP_FAILED)
		goto out;
	rc = uk_vas_clone_process(parent, &child);
	if (rc)
		goto out;
	file_cow_bind(child);
	*(__u32 *)FILE_COW_ADDR = 0x8888U;
	*(__u32 *)FILE_COW_ALIAS = 0x9999U;
	file_cow_bind(parent);
	if (__atomic_load_n((__u32 *)FILE_COW_ADDR, __ATOMIC_RELAXED) !=
	    FILE_COW_INITIAL ||
	    *(__u32 *)FILE_COW_ALIAS != 0x9999U)
		rc = -EIO;
	file_cow_bind(child);
	if (madvise((void *)FILE_COW_ADDR, f.capacity, MADV_DONTNEED) ||
	    *(__u32 *)FILE_COW_ADDR != FILE_COW_INITIAL)
		rc = -EIO;
out:
	file_cow_bind(saved);
	uk_vas_put(child);
	uk_vas_put(parent);
	if (file_cow_fixture_free(&f))
		rc = -EWOULDBLOCK;
	return rc;
}

UK_TESTCASE(file_cow, mapping_resource_balance)
{
	struct uk_pagetable *pt = uk_vas_get_kernel()->pt;
	unsigned int users = pt->shared_users, cycle;
	unsigned long first_free;
	int rc;

	for (cycle = 0; cycle < 2; cycle++) {
		rc = file_cow_balance_cycle();
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			return;
	}
	first_free = pt->fa->free_memory;
	for (cycle = 1; cycle <= 64; cycle++) {
		rc = file_cow_balance_cycle();
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(pt->fa->free_memory, first_free);
		UK_TEST_EXPECT_SNUM_EQ(pt->shared_users, users);
		uk_pr_info("FILE_COW_CYCLE cycle=%u free_bytes=%lu shared_users=%u\n",
			   cycle, pt->fa->free_memory, pt->shared_users);
		if (rc)
			return;
	}
	uk_pr_info("FILE_COW_PASS balance cycles=64\n");
}

/* Fail resident metadata allocations, including partially adopted seeds. */
struct file_cow_alloc {
	struct uk_alloc *real;
	unsigned long live;
	unsigned int calls;
	unsigned int fail_at;
};

static void *file_cow_palloc(struct uk_alloc *a, unsigned long pages)
{
	struct file_cow_alloc *f = (void *)a->priv;
	void *ptr;

	f->calls++;
	if (f->fail_at && f->calls == f->fail_at)
		return NULL;
	ptr = uk_palloc(f->real, pages);
	if (ptr)
		f->live += pages;
	return ptr;
}

static void file_cow_pfree(struct uk_alloc *a, void *ptr, unsigned long pages)
{
	struct file_cow_alloc *f = (void *)a->priv;

	f->live -= pages;
	uk_pfree(f->real, ptr, pages);
}

static int file_cow_alloc_round(struct uk_alloc *a, int fd, size_t len,
				unsigned int fail_at, unsigned int *calls)
{
	struct file_cow_alloc *f = (void *)a->priv;
	struct uk_vas *vas = NULL;
	void *mapped;
	int rc = uk_vas_create_process(a, &vas);

	if (rc)
		return rc;
	f->calls = 0;
	f->fail_at = fail_at;
	mapped = file_cow_map(vas, FILE_COW_ADDR, len, fd, false);
	*calls = f->calls;
	f->fail_at = 0;
	/* No replacement mapping exists, so every injected failure is fatal. */
	rc = (mapped == MAP_FAILED) == (fail_at != 0) ? 0 : -EIO;
	uk_vas_put(vas);
	if (f->live)
		rc = -EIO;
	return rc;
}

UK_TESTCASE(file_cow, allocation_rollback)
{
	struct uk_vas *kernel = uk_vas_get_kernel();
	struct file_cow_fixture fixture;
	struct file_cow_alloc *f;
	struct uk_alloc *a;
	unsigned int total = 0, calls, n, users = kernel->pt->shared_users;
	unsigned long first_free;
	int rc = file_cow_fixture_init_pages(&fixture, 513);

	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	a = uk_malloc(kernel->a, sizeof(*a) + sizeof(*f));
	UK_TEST_EXPECT(!!a);
	if (!a)
		goto out;
	memset_isr(a, 0, sizeof(*a) + sizeof(*f));
	f = (void *)a->priv;
	f->real = kernel->a;
	a->malloc = uk_malloc_ifpages;
	a->free = uk_free_ifpages;
	a->palloc = file_cow_palloc;
	a->pfree = file_cow_pfree;
	/* Warm allocator and source-pool growth before the measured rounds. */
	rc = file_cow_alloc_round(a, fixture.fd[0], fixture.capacity,
				  0, &total);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto free_alloc;
	UK_TEST_EXPECT(total > 3 && total < 256);
	if (total <= 3 || total >= 256)
		goto free_alloc;
	first_free = kernel->pt->fa->free_memory;
	for (n = 1; n <= total; n++) {
		rc = file_cow_alloc_round(a, fixture.fd[0], fixture.capacity,
					  n, &calls);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(calls, n);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, first_free);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->shared_users, users);
		if (rc)
			break;
	}
	if (!rc)
		uk_pr_info("FILE_COW_PASS rollback allocations=%u pages=513\n",
			   total);
free_alloc:
	if (f->live)
		UK_CRASH("File COW test retained allocator pages\n");
	uk_free(kernel->a, a);
out:
	UK_TEST_EXPECT_ZERO(file_cow_fixture_free(&fixture));
}

_UK_TESTSUITE(file_cow, NULL);

static void __constructor run_file_cow(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(file_cow));
}
