/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <uk/essentials.h>
#include <uk/process.h>
#include <uk/syscall.h>

#include "process.h"
#include "signal.h"
#include "sigset.h"

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
UK_LLSYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		      const struct kern_sigaction *, act,
		      struct kern_sigaction *, oldact,
		      size_t, sigsetsize)
{
	struct posix_process *proc;
	struct kern_sigaction action, previous;
	struct uk_list_head discard;
	unsigned long flags;

	if (unlikely(!IS_VALID(signum)))
		return -EINVAL;

	if (unlikely(signum == SIGKILL || signum == SIGSTOP))
		return -EINVAL;

	if (unlikely(sigsetsize != sizeof(uk_sigset_t)))
		return -EINVAL;

	proc = uk_pprocess_current();
	UK_ASSERT(proc);

	UK_INIT_LIST_HEAD(&discard);
	if (act)
		action = *act;
	flags = pprocess_lock_acquire();
	if (oldact)
		previous = *KERN_SIGACTION(proc, signum);
	/* Preserve SA_NOCLDSTOP for readback. There is no job control. */
	if (act)
		*KERN_SIGACTION(proc, signum) = action;
	if (act && IS_IGNORED(proc, signum))
		pprocess_signal_clear_pending_locked(proc, signum, &discard);
	pprocess_lock_release(flags);
	pprocess_signal_free_list(&discard);
	if (oldact)
		*oldact = previous;

	return 0;
}
#else /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
UK_LLSYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		      const struct kern_sigaction *, act,
		      struct kern_sigaction *, oldact,
		      size_t, sigsetsize)
{
	UK_WARN_STUBBED();
	if (oldact)
		*oldact = (struct kern_sigaction){0};

	return 0;
}
#endif /* !CONFIG_LIBPOSIX_PROCESS_SIGNAL */
