/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include <uk/config.h>
#include <uk/arch/lcpu.h>
#include <uk/process.h>
#include <uk/print.h>

#include "signal.h"

#if CONFIG_LIBPOSIX_PROCESS_CLONE
static int uk_posix_clone_sighand(const struct clone_args *cl_args,
				  size_t cl_args_len __unused,
				  struct uk_thread *child __unused,
				  struct uk_thread *parent __unused)
{
	struct posix_process *pp; /* parent process */
	struct posix_process *cp; /* child process */
	struct posix_thread *ct; /* child thread */
	pid_t ppid; /* parent pid */
	pid_t cpid; /* child pid */
	int rc;

	ppid = ukthread2pid(parent);
	cpid = ukthread2pid(child);

	pp = pid2pprocess(ppid);
	cp = pid2pprocess(cpid);

	ct = tid2pthread(ukthread2tid(child));

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

	/* Initialize the new thread's signal descriptor */
	rc = pprocess_signal_tdesc_alloc(ct);
	if (unlikely(rc)) {
		uk_pr_err("Could not allocate signal descriptor\n");
		return rc;
	}
	rc = pprocess_signal_tdesc_init(ct);
	if (unlikely(rc)) {
		uk_pr_err("Could not initialize signal descriptor\n");
		return rc;
	}

	/* If CLONE_THREAD was passed, the child is assigned to the
	 * calling process, so no further action is required.
	 */
	if (cl_args->flags & CLONE_THREAD)
		return 0;

	/* Initialize the new process' signal descriptor */
	cp->signal = uk_malloc(cp->_a, sizeof(struct uk_signal_tdesc));
	if (unlikely(!cp->signal)) {
		uk_pr_err("Could not allocate memory\n");
		return -ENOMEM;
	}

	/* CLONE_CLEAR_SIGHAND: Reset child's signal dispositions to default. */
	if (cl_args->flags & CLONE_CLEAR_SIGHAND) {
		uk_pr_debug("CLONE_SIGHAND: pid %d gets default signal dispositions\n",
			    cpid);

		if (!(cl_args->flags & CLONE_THREAD)) {
			cp->signal->sigaction = uk_malloc(ct->_a,
							  SIG_ARRAY_COUNT);
			if (unlikely(!cp->signal->sigaction)) {
				uk_pr_err("Could not allocate memory\n");
				return -ENOMEM;
			}
		}

		for (__sz i = 0; i < SIG_ARRAY_COUNT; i++)
			pprocess_signal_sigaction_clear(&cp->signal->sigaction[i]);

	/* CLONE_SIGHAND: Inherit a reference of the parent's signal handler
	 *                table.
	 */
	} else if (cl_args->flags & CLONE_SIGHAND) {
		uk_pr_debug("CLONE_SIGHAND: pid %d gets a reference of the parent's handlers\n",
			    cpid);

		cp->signal->sigaction = pp->signal->sigaction;

	/* Default: Iherit a copy of the parent's signal dispositions. */
	} else {
		uk_pr_debug("pid %d gets a copy of the parent's handlers\n",
			    cpid);

		cp->signal->sigaction = uk_malloc(ct->_a, SIG_ARRAY_COUNT);
		if (unlikely(!cp->signal->sigaction)) {
			uk_pr_err("Could not allocate memory\n");
			return -ENOMEM;
		}
		memcpy(&cp->signal->sigaction, &cp->signal->sigaction,
		       SIG_ARRAY_COUNT * sizeof(struct k_sigaction));
	}

	/* The child process has no pending signals */
	cp->signal->queued_count = 0;
	cp->signal->queued_max = _POSIX_SIGQUEUE_MAX;

	uk_sigemptyset(&cp->signal->sigqueue.pending);
	for (__sz i = 0; i < SIG_ARRAY_COUNT; i++)
		UK_INIT_LIST_HEAD(&cp->signal->sigqueue.list_head[i]);

	/* sigaltstack(2): Children created with clone() inherit the
	 * parent's altstack settings, unless clone() was passed the
	 * CLONE_VM and not CLONE_VFORK. In that case the altstack
	 * inherited by the parent is disabled.
	 */
	memcpy(&cp->signal->altstack, &pp->signal->altstack, sizeof(stack_t));
	if ((cl_args->flags & CLONE_VM) && !(cl_args->flags & CLONE_VFORK))
		cp->signal->altstack.ss_flags = SS_DISABLE;

	return 0;
}

UK_POSIX_CLONE_HANDLER(CLONE_SIGHAND | CLONE_CLEAR_SIGHAND, false,
		       uk_posix_clone_sighand, 0x0);
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */
