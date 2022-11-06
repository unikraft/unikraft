#include "test_clone_helper.h"

/*
 * This test will check the uk_sched_thread_exit2() call
 * which will allow unmapping memory for detached threads.
 * Detached threads are threads that no one is waiting for.
 */

void *__detached_child_map;
size_t __detached_child_map_size;
unsigned long __detached_child_tls_area;
unsigned long __detached_child_tls_ptr;
int UNMAP_CB_CALLED;
struct uk_thread *__detached_child_uk_thread_ptr;

/* The following code is copied from __uk_unmapself.c from musl. */
struct __map_area {
	void *map_base;
	size_t map_size;
};

/*
 * This will be called in the context of the `idle` thread.
 */
static void __uk_unmapself_cb(struct uk_thread *t, void *mapped_area)
{
	struct __map_area *m = (struct __map_area *) mapped_area;

	munmap(m->map_base, m->map_size);
	UNMAP_CB_CALLED = 1;
}

static __uk_tls struct __map_area __map_area = {};
static void __local_unmapself(void *base, size_t size)
{
	/*
	 * In order not to allocate memory (e.g. with malloc()) that could fail,
	 * we will use a variable in TLS to store the map address and size.
	 * When the callback will be run from the context of the `idle` thread,
	 * the TLS of this thread will be accessed, after which everything
	 * will be deallocated.
	 */
	__map_area.map_base = base;
	__map_area.map_size = size;

	uk_sched_thread_exit2(__uk_unmapself_cb, &__map_area);
}
/************************************************************************/

static int detached_fun(void *args)
{
	__detached_child_uk_thread_ptr = uk_thread_current();

	/* The thread will execute a different exit() call,
	 * which will allow "self" unmapping (although is not real self
	 * unmapping).
	 */
	__local_unmapself(__detached_child_map, __detached_child_map_size);

	return 0;
}

UK_TESTCASE(posix_process_test_detached_testsuite, detached_child)
{
	/* In this test we're trying to do what musl does for detached
	 * threads.
	 */
	/* Layout of the mapping x86_64
	 * (there is also some padding but we omit it):
	 * map -----------------------------------------------------------------
	 *                     ^
	 *                     | STACK
	 *                     v
	 * stack ---------------------------------------------------------------
	 *                         ^           ^
	 *                         |           | TLS SPACE
	 *                         |           v
	 * ukplat_tlsp_get(),tcb ->|-TLS AREA-----------------------------------
	 *                         |           ^
	 *                         |           | LIBC TCB(e.g pthread structure)
	 *                         v           v
	 *----------------------------------------------------------------------
	 *
	 *
	 * Layout of the mapping aarch64
	 * (there is also some padding but we omit it):
	 * map -----------------------------------------------------------------
	 *                    ^
	 *                    |           STACK
	 *                    v
	 * stack ---------------------------------------------------------------
	 *                  tcb -> ^           ^
	 *                         |           | LIBC tcb(e.g pthread structure)
	 *                         |           v
	 *      ukplat_tlsp_get()->|-TLS AREA-----------------------------------
	 *                         |           ^
	 *                         |           | TLS SPACE
	 *                         v           v
	 *----------------------------------------------------------------------
	 */
	size_t stack_size = 8 * 4096; // not using the default size
	size_t tls_size = ukarch_tls_area_size();

	__detached_child_map = mmap(NULL, stack_size + tls_size,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	__detached_child_map_size = stack_size + tls_size;
	__detached_child_tls_area = (unsigned long) __detached_child_map
		+ stack_size;
	/* Function defined in __uk_init_tls.c glue src file (musl). */
	void *tcb = __uk_copy_tls((void *) __detached_child_tls_area);
	void *stack = (void *) __detached_child_tls_area;
	int ctid;

	/* These flags are used by pthread_create() when a new thread is
	 * createad.
	 */
	unsigned int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
		| CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS
		| CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_DETACHED;

	__detached_child_tls_ptr = (unsigned long)
		ukarch_tls_tlsp((void *) __detached_child_tls_area);
	/* Use the clone wrapper from musl. */
	__clone((int (*)(void *)) detached_fun, stack, flags, tcb, &ctid,
		__detached_child_tls_ptr, &ctid);
	/* Clone will force the execution of child after it creates it,
	 * but just to be sure the child executes and finishes,
	 * we place some sleeps here.
	 */
	unsigned int no_of_tries = 5;

	while (no_of_tries--) {
		if (!__check_if_child_exited(__detached_child_uk_thread_ptr))
			sleep(1);
		else
			break;
	}

	UK_TEST_EXPECT_NOT_ZERO(
		__check_if_child_exited(__detached_child_uk_thread_ptr));

	no_of_tries = 5;
	while (no_of_tries-- && !UNMAP_CB_CALLED) {
		/* let the idle thread run */
		sleep(2);
	}

	UK_TEST_ASSERTF(UNMAP_CB_CALLED == 1,
		"expected uksched_thread_exit2 to work, UNMAP_CB_CALLED = %d",
		UNMAP_CB_CALLED);
}

uk_testsuite_register(posix_process_test_detached_testsuite, NULL);
