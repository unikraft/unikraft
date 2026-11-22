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

int pprocess_signal_clone(struct posix_thread *ct, struct posix_thread *pt,
			  __u64 flags)
{
	struct posix_process *pp; /* parent process */
	struct posix_process *cp; /* child process */
	pid_t cpid; /* child pid */
	int signum;

	UK_ASSERT(ct && ct->signal && ct->process && ct->process->signal);
	cp = ct->process;
	pp = pt ? pt->process : __NULL;
	cpid = cp->pid;

	/* CLONE_SIGHAND and CLONE_CLEAR_SIGHAND should not be together */
	if (unlikely((flags & (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND))
		     == (CLONE_SIGHAND | CLONE_CLEAR_SIGHAND)))
		return -EINVAL;

	/* CLONE_SIGHAND requires CLONE_VM */
	if (unlikely((flags & CLONE_SIGHAND) && !(flags & CLONE_VM)))
		return -EINVAL;

	/* CLONE_THREAD requires CLONE_SIGHAND */
	if (unlikely((flags & CLONE_THREAD) && !(flags & CLONE_SIGHAND)))
		return -EINVAL;

	/* Masks belong to individual threads, even with shared dispositions.
	 * Keep the initialized child pending queues and synchronization state.
	 */
	if (pt) {
		UK_ASSERT(pt->signal);
		memcpy(&ct->signal->mask, &pt->signal->mask,
		       sizeof(ct->signal->mask));
	}

	/* If CLONE_THREAD was passed, the child is assigned to the
	 * calling process, so no further action is required.
	 */
	if (flags & CLONE_THREAD)
		return 0;

	/* Process/thread creation already initialized these descriptors and
	 * the child's private default action table. Keep their ownership.
	 */

	/* CLONE_CLEAR_SIGHAND: Reset child's signal dispositions to default. */
	if (!pp || (flags & CLONE_CLEAR_SIGHAND)) {
		uk_pr_debug("CLONE_SIGHAND: pid %d gets default signal dispositions\n",
			    cpid);

		pprocess_signal_foreach(signum)
			pprocess_signal_sigaction_clear(KERN_SIGACTION(cp, signum));

	/* CLONE_SIGHAND: Inherit a reference of the parent's signal handler
	 *                table.
	 */
	} else if (flags & CLONE_SIGHAND) {
		uk_pr_debug("CLONE_SIGHAND: pid %d gets a reference of the parent's handlers\n",
			    cpid);
		pprocess_signal_sigaction_acquire(pp->signal->sigaction);
		pprocess_signal_sigaction_release(cp->signal->sigaction);
		cp->signal->sigaction = pp->signal->sigaction;
	/* Default: Iherit a copy of the parent's signal dispositions. */
	} else {
		uk_pr_debug("pid %d gets a copy of the parent's handlers\n",
			    cpid);

		memcpy(cp->signal->sigaction->ks, pp->signal->sigaction->ks,
		       sizeof(cp->signal->sigaction->ks));
	}

	if (pp) {
		/* sigaltstack(2): Children created with clone() inherit the
		 * parent's altstack settings, unless clone() was passed the
		 * CLONE_VM and not CLONE_VFORK. In that case the altstack
		 * inherited by the parent is disabled.
		 */
		memcpy(&cp->signal->altstack, &pp->signal->altstack, sizeof(stack_t));
		if ((flags & CLONE_VM) && !(flags & CLONE_VFORK))
			cp->signal->altstack.ss_flags = SS_DISABLE;
	}

	return 0;
}

static int uk_posix_clone_sighand(void *arg)
{
	struct posix_process_clone_event_data *event_data = arg;
	struct posix_thread *ct, *pt;

	UK_ASSERT(event_data && event_data->cl_args && event_data->child);
	ct = tid2pthread(ukthread2tid(event_data->child));
	pt = event_data->parent ?
		tid2pthread(ukthread2tid(event_data->parent)) : __NULL;
	return pprocess_signal_clone(ct, pt, event_data->cl_args->flags);
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
