/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <stddef.h>
#include <sys/types.h>
#include <uk/syscall.h>

#include "signal.h"

UK_SYSCALL_R_DEFINE(int, pause)
{
	struct posix_thread *this_thread;

	this_thread = tid2pthread(uk_gettid());

	/* Return as soon as a signal is queued. The signal will
	 * be delivered at syscall exit.
	 */
	this_thread->state = POSIX_THREAD_BLOCKED_SIGNAL;
	uk_semaphore_down(&this_thread->signal->deliver_semaphore);

	return -EINTR;
}
