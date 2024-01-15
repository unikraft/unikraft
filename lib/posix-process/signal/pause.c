/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <sys/types.h>
#include <uk/syscall.h>

#include "process.h"
#include "signal.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_SYSCALL_R_DEFINE(int, pause)
{
	struct posix_thread *pthread;

	pthread = uk_pthread_current();
	UK_ASSERT(pthread);

	/* Return as soon as a signal is queued. The signal will
	 * be delivered at syscall exit.
	 */
	pthread->state = POSIX_THREAD_BLOCKED_SIGNAL;
	uk_semaphore_down(&pthread->signal->deliver_semaphore);

	return -EINTR;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_SYSCALL_R_DEFINE(int, pause)
{
	UK_WARN_STUBBED();
	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
