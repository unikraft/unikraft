#include "test_syscall_shim.h"

char TLS_REGION[100];
unsigned long return_addr;
int is_binary;

unsigned long ktls;
unsigned long ultls;
unsigned long saved_return_addr;
/*
 * This syscall is only for testing the User land TLS.
 * Not a real fork !!!
 */
UK_SYSCALL_R_DEFINE(int, fork)
{
	if (is_binary) {
		struct uk_thread *self = uk_thread_current();

		ktls = ukplat_tlsp_get();
		if (self->uktlsp != ktls)
			return 0;

		if (ktls == (unsigned long) TLS_REGION)
			return 0;

		ultls = uk_syscall_ultlsp();
	}

	saved_return_addr = uk_syscall_return_addr();

	return 0;
}

UK_TESTCASE(posix_process_syscall_shim_testsuite, test_fork)
{
	/* Save the real tls. */
	unsigned long orig = ukplat_tlsp_get();
	unsigned long utls_syscall_ret;

	/* Mimics an userland tls. */
	ukplat_tlsp_set((unsigned long) TLS_REGION);

	UK_TEST_ASSERTF(((void *) ukplat_tlsp_get() == TLS_REGION),
		"expected ULTLS 0x%lx to be set to %p", ukplat_tlsp_get(),
		TLS_REGION);

	is_binary = 1;
	call_binary_fork();

	utls_syscall_ret = ukplat_tlsp_get();

	/* Restore the original tls. */
	ukplat_tlsp_set(orig);

	UK_TEST_ASSERTF((uk_thread_current()->uktlsp == ktls),
		"expected KTLS 0x%lx == self->uktlsp %p",
		ktls, uk_thread_current()->uktlsp);

	UK_TEST_ASSERTF((ktls != (unsigned long) TLS_REGION),
		"expected KTLS 0x%lx != ULTLS %p", ktls, TLS_REGION);

	UK_TEST_ASSERTF((ultls == (unsigned long) TLS_REGION),
		"expected uk_syscall_ultlsp() to give %p of ULTLS", TLS_REGION);

	UK_TEST_ASSERTF((return_addr == saved_return_addr),
		"expected return_addr 0x%lx == uk_syscall_return_addr() 0x%lx",
		return_addr, saved_return_addr);

	/* When we return from syscall the ULTLS pointer should be restored. */
	UK_TEST_ASSERTF((utls_syscall_ret == (unsigned long) TLS_REGION),
		"expected ULTLS %p == 0x%lx after syscall return",
		TLS_REGION, utls_syscall_ret);

	is_binary = 0;
	call_fork();

	UK_TEST_ASSERTF((return_addr == saved_return_addr),
		"expected return_addr 0x%lx == uk_syscall_return_addr() 0x%lx",
		return_addr, saved_return_addr);
}

uk_testsuite_register(posix_process_syscall_shim_testsuite, NULL);
