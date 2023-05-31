/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * Parts are copyright by other contributors. Please refer to copyright notices
 * in the individual source files, and to the git commit log, for a more accurate
 * list of copyright holders.
 *
 * OSv is open-source software, distributed under the 3-clause BSD license:
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *       this list of conditions and the following disclaimer in the documentation
 *       and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Cloudius Systems, Ltd. nor the names of its
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* adapted from OSv */

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <uk/config.h>
#include <uk/essentials.h>
#include <uk/syscall.h>
#ifndef __NEED_struct_timespec
#define __NEED_struct_timespec
#endif
#include <sys/types.h>
#include "sigset.h"
#include "ksigaction.h"

UK_SYSCALL_R_DEFINE(int, sigaltstack, const stack_t *, ss,
		    stack_t *, old_ss)
{
	return 0;
}

UK_SYSCALL_R_DEFINE(int, rt_sigaction, int, signum,
		    const struct k_sigaction *__unused, act,
		    struct k_sigaction *, oldact,
		    size_t __unused, sigsetsize)
{
	if (unlikely(signum == SIGKILL || signum == SIGSTOP))
		return -EINVAL;

	if (oldact)
		*oldact = (struct k_sigaction){0};

	return 0;
}

#if UK_LIBC_SYSCALLS
int sigaction(int signum, const struct sigaction __unused *act,
              struct sigaction *oldact)
{
	/* Not actually an implementation of sigaction.
	 * Do minimal argument matching for the stub syscall. */
	int r;
	struct k_sigaction kold;
	r = rt_sigaction(signum, NULL, oldact ? &kold : NULL, sizeof(sigset_t));
	if (oldact && !r) {
		oldact->sa_handler = kold.handler;
		oldact->sa_flags = kold.flags;
	}
	return r;
}

sighandler_t signal(int signum, sighandler_t handler)
{
	struct k_sigaction old;
	struct k_sigaction act = {
		.handler = handler,
		.flags = SA_RESTART, /* BSD signal semantics */
	};

	if (unlikely(rt_sigaction(signum, &act, &old, sizeof(sigset_t)) < 0))
		return SIG_ERR;

	return (old.flags & SA_SIGINFO) ? NULL : old.handler;
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, rt_sigpending,
		    sigset_t *, set,
		    size_t __unused, sigsetsize)
{
	sigemptyset(set);

	return 0;
}

#if UK_LIBC_SYSCALLS
int sigpending(sigset_t *set)
{
	return rt_sigpending(set, sizeof(sigset_t));
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, rt_sigprocmask,
		    int __unused, how,
		    const sigset_t *__unused, set,
		    sigset_t *__unused, oldset,
		    size_t __unused, sigsetsize)
{
	return 0;
}

#if UK_LIBC_SYSCALLS
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	return rt_sigprocmask(how, set, oldset, sizeof(sigset_t));
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, rt_sigsuspend,
		    const sigset_t *__unused, mask,
		    size_t __unused, sigsetsize)
{
	return -EINTR;
}

#if UK_LIBC_SYSCALLS
int sigsuspend(const sigset_t *mask)
{
	return rt_sigsuspend(mask, sizeof(sigset_t));
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, rt_sigtimedwait,
		    const sigset_t *__unused, set,
		    siginfo_t *, info,
		    const struct timespec *__unused, timeout,
		    size_t __unused, sigsetsize)
{
	*info = (siginfo_t){0};
	info->si_signo = SIGINT;

	return 0;
}

#if UK_LIBC_SYSCALLS
int sigwait(const sigset_t *set, int *sig)
{
	int error;
	siginfo_t si;

	error = rt_sigtimedwait(set, &si, NULL, sizeof(sigset_t));
	*sig = si.si_signo;

	return error;
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, tkill,
		    int __unused, tid,
		    int __unused, sig)
{
	return 0;
}

#if UK_LIBC_SYSCALLS
int raise(int sig)
{
	return tkill(-1, sig);
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, kill,
		    pid_t, pid,
		    int __unused, sig)
{
	if (unlikely(pid != 0))
		return -ESRCH;

	return 0;
}

#if UK_LIBC_SYSCALLS
int killpg(int pgrp, int sig)
{
	return kill(-pgrp, sig);
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(unsigned int, alarm,
		    unsigned int __unused, seconds)
{
	return 0;
}

#if UK_LIBC_SYSCALLS
int siginterrupt(int sig __unused, int flag __unused)
{
	return 0;
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, pause)
{
	return 0;
}
