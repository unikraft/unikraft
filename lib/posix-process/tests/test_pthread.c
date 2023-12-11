#include "test_clone_helper.h"


/* This test will check the basic actions from the pthread API
 * which rely on clone(), exit() (NB:SYS_exit).
 */
void *__pthread_basic_child_self_ptr;
void *function_arg;

static void *pthread_basic_thread_fun(void *args)
{
	function_arg = args;

	__pthread_basic_child_self_ptr = pthread_self();

	return (void *) 0xbadbabee;
}

UK_TESTCASE(posix_process_test_pthread_testsuite, basic_create_join)
{
	pthread_t thread_id;
	void *retval;

	/* This will invoke clone(). */
	pthread_create(&thread_id, NULL, pthread_basic_thread_fun,
		(void *) 0xdeadbeef);
	/* This will wait for the thread to do exit(). */
	pthread_join(thread_id, &retval);

	UK_TEST_EXPECT_SNUM_EQ(function_arg, (void *) 0xdeadbeef);

	/* The address in thread_id is not changed after pthread_join. */
	UK_TEST_EXPECT_SNUM_EQ(__pthread_basic_child_self_ptr,
		(void *) thread_id);

	UK_TEST_EXPECT_SNUM_EQ(retval, (void *) 0xbadbabee);
}

uk_testsuite_register(posix_process_test_pthread_testsuite, NULL);

