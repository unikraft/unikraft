#include "test_clone_helper.h"

/* This test will check the clone syscall behaves correctly */

int main_thread_tid;
int __clone_child_thread_tid;
unsigned long __clone_child_tls_area;
unsigned long __clone_child_tls_ptr;
struct uk_thread *__clone_child_uk_thread_ptr;
__thread unsigned long per_thread_var = 0xdeadbeef;

unsigned long per_thread_var_addr;
unsigned long tls_start;
unsigned long tls_end;
unsigned long actual_clone_child_tls_ptr;

static int clone_thread_fun(void *arg)
{
	__clone_child_uk_thread_ptr = uk_thread_current();
	per_thread_var_addr = (unsigned long) &per_thread_var;

	__clone_child_thread_tid = syscall(SYS_gettid);

	actual_clone_child_tls_ptr = ukplat_tlsp_get();

	tls_start = __clone_child_tls_area;
	tls_end = __clone_child_tls_area
		+ (unsigned long) ukarch_tls_area_size();

	per_thread_var = 0xbadbabee;

	return 0;
}


UK_TESTCASE(posix_process_test_clone_testsuite, single_child)
{
	/* In this test we're trying to do what musl does with the clone
	 * syscall.
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
	__u8 *map = mmap(NULL, stack_size + tls_size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	__clone_child_tls_area = (unsigned long) map + stack_size;
	/* Function defined in __uk_init_tls.c glue src file (musl). */
	void *tcb = __uk_copy_tls((void *) __clone_child_tls_area);
	void *stack = map +  stack_size;
	int ctid;

	main_thread_tid = syscall(SYS_gettid);

	/* These flags are used by pthread_create() when a new thread is
	 * createad.
	 */
	unsigned int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
		| CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS
		| CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_DETACHED;

	__clone_child_tls_ptr = (unsigned long)
		ukarch_tls_tlsp((void *) __clone_child_tls_area);
	/* Use the clone wrapper from musl.
	 * NB: this will call SYS_exit after clone_thread_fun() returns
	 */
	__clone(clone_thread_fun, stack, flags, tcb, &ctid,
		__clone_child_tls_ptr, &ctid);
	/* Clone will force the execution of child after it creates it,
	 * but just to be sure the child executes and finishes,
	 * we place some sleeps here.
	 */
	unsigned int no_of_tries = 5;

	while (no_of_tries--) {
		if (!__check_if_child_exited(__clone_child_uk_thread_ptr))
			sleep(1);
		else
			break;
	}

	UK_TEST_EXPECT_NOT_ZERO(
		__check_if_child_exited(__clone_child_uk_thread_ptr));

	UK_TEST_EXPECT_SNUM_NQ(__clone_child_thread_tid, main_thread_tid);
	UK_TEST_EXPECT_SNUM_EQ(__clone_child_tls_ptr,
		actual_clone_child_tls_ptr);

	UK_TEST_ASSERTF(per_thread_var_addr >= tls_start,
		"expected tls_start:0x%lx <= &per_thread_var:0x%lx",
		tls_start, per_thread_var_addr);

	UK_TEST_ASSERTF(per_thread_var_addr <= tls_end,
		"expected &per_thread_var:0x%lx <= tls_end:0x%lx",
		per_thread_var_addr, tls_end);

#ifdef CONFIG_ARCH_X86_64
	UK_TEST_ASSERTF(per_thread_var_addr <= __clone_child_tls_ptr,
		"expected &per_thread_var:0x%lx <= tlsp:0x%lx",
		per_thread_var_addr, __clone_child_tls_ptr);
#endif
#ifdef CONFIG_ARCH_ARM_64
	UK_TEST_ASSERTF(per_thread_var_addr >= __clone_child_tls_ptr,
		"expected &per_thread_var:0x%lx >= tlsp:0x%lx",
		per_thread_var_addr, __clone_child_tls_ptr);
#endif

	/* At this point the child exited, so we need to check if the tid is set
	 * to 0.
	 * This must happen because one of the flags that we gave to clone() is
	 * CLONE_CHILD_CLEARTID.
	 */
	UK_TEST_ASSERTF(ctid == 0 && ctid != __clone_child_thread_tid,
		"expected ctid %d == 0 && ctid != __clone_child_thread_tid %d",
		ctid, __clone_child_thread_tid);

	/* Check if per_thread_var is really per thread. */
	UK_TEST_EXPECT_SNUM_EQ(per_thread_var, 0xdeadbeef);
}

uk_testsuite_register(posix_process_test_clone_testsuite, NULL);
