/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/event.h>
#include <uk/prio.h>
#include <uk/process.h>
#include <uk/syscall.h>
#include <uk/thread.h>

static int syscall_nested_depth_reset(void *data)
{
	struct posix_process_execve_event_data *event_data;

	UK_ASSERT(data);

	event_data = (struct posix_process_execve_event_data *)data;

	UK_ASSERT(event_data->thread);

	/* execve() will always enter ukplat_syscall_handler() with a
	 * sane depth value, that is:
	 *
	 * - when preceded by a vfork() we start off with a new uktls,
	 *   and hence enter ukplat_sycall_handler() with zero depth.
	 * - when NOT preceded by a vfork(), the depth should have a
	 *   sane value from the previous syscall, that is we also enter
	 *   uk_syscall_handler() with a depth of zero.
	 *
	 * Nevertheless, since execve() does not go through the syscall return
	 * path we must reset its counter back to zero here so that the
	 * next syscall starts off with a sane value.
	 */
	uk_thread_uktls_var(event_data->thread, uk_syscall_nested_depth) = 0;

	return UK_EVENT_HANDLED_CONT;
}

POSIX_PROCESS_EXECVE_HANDLER_PRIO(syscall_nested_depth_reset, UK_PRIO_EARLIEST);
