/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <string.h>

#include <uk/config.h>
#include <uk/lcpu.h>
#include <uk/process.h>
#include <uk/print.h>

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL

#include "signal.h"

static
int uk_posix_clone_sighand(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	const struct clone_args *cl_args;
	struct posix_process *pp; /* parent process */
	struct posix_process *cp; /* child process */
	struct posix_thread *ct; /* child thread */
	pid_t ppid; /* parent pid */
	pid_t cpid; /* child pid */

	event_data = (struct posix_process_clone_event_data *)arg;
	UK_ASSERT(event_data);
	UK_ASSERT(event_data->cl_args);
	UK_ASSERT(event_data->child);

	cl_args = event_data->cl_args;

	cpid = ukthread2pid(event_data->child);
	cp = pid2pprocess(cpid);

	if (event_data->parent) {
		ppid = ukthread2pid(event_data->parent);
		pp = pid2pprocess(ppid);
	} else {
		pp = __NULL;
	}

	ct = tid2pthread(ukthread2tid(event_data->child));

	/* CLONE_SIGHAND and CLONE_CLEAR_SIGHAND should not be together */
	if (unlikely((cl_args->flags & (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND))
		     == (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND)))
		return -EINVAL;

	/* CLONE_SIGHAND requires CLONE_VM */
	if (unlikely((cl_args->flags & CLONE_SIGHAND) &&
		     !(cl_args->flags & CLONE_VM)))
		return -EINVAL;

	/* CLONE_THREAD requires CLONE_SIGHAND */
	if (unlikely((cl_args->flags & CLONE_THREAD) &&
		     !(cl_args->flags & CLONE_SIGHAND)))
		return -EINVAL;

	/* The new thread's signal descriptor is the one it was made with (see
	 * pprocess_create_pthread())
	 */
	UK_ASSERT(ct->signal);

	/* If CLONE_THREAD was passed, the child is assigned to the
	 * calling process, so no further action is required.
	 */
	if (cl_args->flags & CLONE_THREAD)
		return 0;

	/* The new process' signal descriptor is the one it was made with (see
	 * pprocess_signal_pdesc_init()): default dispositions in a table of
	 * its own, nothing pending, no alternate signal stack, an empty
	 * signal file context.
	 *
	 * TODO: At this time we do not support proper open file inheritance
	 * across vfork/execve for those that do not have the O_CLOEXEC flag.
	 * Therefore, for now, the child process' signal file context stays
	 * empty.
	 */
	UK_ASSERT(cp->signal);

	/* No parent, or CLONE_CLEAR_SIGHAND: the child keeps the default
	 * signal dispositions it was made with.
	 */
	if (pp && !(cl_args->flags & CLONE_CLEAR_SIGHAND)) {
		if (cl_args->flags & CLONE_SIGHAND) {
			/* CLONE_SIGHAND: Inherit a reference of the parent's
			 * signal handler table.
			 */
			uk_pr_debug("CLONE_SIGHAND: pid %d gets a reference of the parent's handlers\n",
				    cpid);
			pprocess_signal_sigaction_release(cp->signal->sigaction);
			pprocess_signal_sigaction_acquire(pp->signal->sigaction);
			cp->signal->sigaction = pp->signal->sigaction;
		} else {
			/* Default: Inherit a copy of the parent's signal
			 * dispositions; the handlers only: the table's
			 * allocator and reference count are the new table's
			 * own.
			 */
			uk_pr_debug("pid %d gets a copy of the parent's handlers\n",
				    cpid);
			memcpy(cp->signal->sigaction->ks,
			       pp->signal->sigaction->ks,
			       sizeof(cp->signal->sigaction->ks));
		}
	}

	if (pp) {
		/* sigaltstack(2): Children created with clone() inherit the
		 * parent's altstack settings, unless clone() was passed the
		 * CLONE_VM and not CLONE_VFORK. In that case the altstack
		 * inherited by the parent is disabled.
		 */
		memcpy(&cp->signal->altstack, &pp->signal->altstack, sizeof(stack_t));
		if ((cl_args->flags & CLONE_VM) && !(cl_args->flags & CLONE_VFORK))
			cp->signal->altstack.ss_flags = SS_DISABLE;
	}

	return 0;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
static int uk_posix_clone_sighand(void *arg)
{
	struct posix_process_clone_event_data *event_data;
	const struct clone_args *cl_args;

	event_data = (struct posix_process_clone_event_data *)arg;
	UK_ASSERT(event_data);
	UK_ASSERT(event_data->cl_args);

	cl_args = event_data->cl_args;

	/* CLONE_SIGHAND and CLONE_CLEAR_SIGHAND should not be together */
	if (unlikely((cl_args->flags & (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND)) ==
		     (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND)))
		return -EINVAL;
	/* CLONE_SIGHAND requires CLONE_VM */
	if (unlikely((cl_args->flags & CLONE_SIGHAND) &&
		     !(cl_args->flags & CLONE_VM)))
		return -EINVAL;
	/* CLONE_THREAD requires CLONE_SIGHAND */
	if (unlikely((cl_args->flags & CLONE_THREAD) &&
		     !(cl_args->flags & CLONE_SIGHAND)))
		return -EINVAL;

	UK_WARN_STUBBED();
	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */

POSIX_PROCESS_CLONE_HANDLER(CLONE_SIGHAND | CLONE_CLEAR_SIGHAND,
			    uk_posix_clone_sighand);
