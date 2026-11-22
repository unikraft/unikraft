/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/syscall.h>

#if UK_LIBC_SYSCALLS
.global fork
fork:
	jmp	uk_syscall_e_fork
#endif /* UK_LIBC_SYSCALLS */
