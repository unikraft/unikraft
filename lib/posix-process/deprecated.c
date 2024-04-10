/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Felipe Huici <felipe.huici@neclab.eu>
 *          Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation. All rights reserved.
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <uk/process.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/arch/limits.h>
#if CONFIG_LIBVFSCORE
#include <vfscore/file.h>
#endif

#define UNIKRAFT_SID      0
#define UNIKRAFT_PGID     0
#define UNIKRAFT_PROCESS_PRIO 0

static void exec_warn_argv_variadic(const char *arg, va_list args)
{
	int i = 1;
	char *argi;

	uk_pr_warn(" argv=[%s", arg);

	argi = va_arg(args, char *);
	while (argi) {
		uk_pr_warn("%s%s", (i > 0 ? ", " : ""), argi);
		i++;
		argi = va_arg(args, char *);
	}
	uk_pr_warn("]\n");
}

static void __exec_warn_array(const char *name, char *const argv[])
{
	uk_pr_warn(" %s=[", name);

	if (argv) {
		int i = 0;
		while (argv[i]) {
			uk_pr_warn("%s%s", (i > 0 ? ", " : ""), argv[i]);
			i++;
		}
	}
	uk_pr_warn("]\n");
}
#define exec_warn_argv(values) __exec_warn_array("argv", values)
#define exec_warn_envp(values) __exec_warn_array("envp", values)

int execl(const char *path, const char *arg, ...
		/* (char  *) NULL */)
{
	va_list args;

	uk_pr_warn("%s(): path=%s\n", __func__, path);
	va_start(args, arg);
	exec_warn_argv_variadic(arg, args);
	va_end(args);
	errno = ENOSYS;
	return -1;
}

int execlp(const char *file, const char *arg, ...
		/* (char  *) NULL */)
{
	va_list args;

	uk_pr_warn("%s(): file=%s\n", __func__, file);
	va_start(args, arg);
	exec_warn_argv_variadic(arg, args);
	va_end(args);
	errno = ENOSYS;
	return -1;
}

int execle(const char *path, const char *arg, ...
		/*, (char *) NULL, char * const envp[] */)
{
	va_list args;
	char * const *envp;

	uk_pr_warn("%s(): path=%s\n", __func__, path);
	va_start(args, arg);
	exec_warn_argv_variadic(arg, args);
	envp = va_arg(args, char * const *);
	exec_warn_envp(envp);
	va_end(args);
	errno = ENOSYS;
	return -1;
}

int execv(const char *path, char *const argv[])
{
	uk_pr_warn("%s(): path=%s\n", __func__, path);
	exec_warn_argv(argv);
	errno = ENOSYS;
	return -1;
}

int execvp(const char *file, char *const argv[])
{
	uk_pr_warn("%s(): file=%s\n", __func__, file);
	exec_warn_argv(argv);
	errno = ENOSYS;
	return -1;
}

int execvpe(const char *file, char *const argv[], char *const envp[])
{
	uk_pr_warn("%s(): file=%s\n", __func__, file);
	exec_warn_argv(argv);
	exec_warn_envp(envp);
	errno = ENOSYS;
	return -1;
}

int system(const char *command)
{
	uk_pr_warn("%s: %s\n", __func__, command);
	errno = ENOSYS;
	return -1;
}

FILE *popen(const char *command, const char *type __unused)
{
	uk_pr_warn("%s: %s\n", __func__, command);
	errno = ENOSYS;
	return NULL;
}

int pclose(FILE *stream __unused)
{
	errno = EINVAL;
	return -1;
}

UK_SYSCALL_R_DEFINE(pid_t, setsid)
{
	/* We have a single "session" with a single "process" */
	return (pid_t) -EPERM;
}

UK_SYSCALL_R_DEFINE(pid_t, getsid, pid_t, pid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		return (pid_t) -ESRCH;
	}
	return UNIKRAFT_SID;
}

UK_SYSCALL_R_DEFINE(int, setpgid, pid_t, pid, pid_t, pgid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		return (pid_t) -ESRCH;
	}
	if (pgid != 0) {
		/* We have a single "group" with a single "process" */
		return (pid_t) -EPERM;
	}
	return 0;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgid, pid_t, pid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		return (pid_t) -ESRCH;
	}
	return UNIKRAFT_PGID;
}

UK_SYSCALL_R_DEFINE(pid_t, getpgrp)
{
	return UNIKRAFT_PGID;
}

#if UK_LIBC_SYSCALLS
int setpgrp(void)
{
	return setpgid(0, 0);
}
#endif /* UK_LIBC_SYSCALLS */

#if UK_LIBC_SYSCALLS
int tcsetpgrp(int fd __unused, pid_t pgrp)
{
	/* TODO check if fd is BADF */
	if (pgrp != UNIKRAFT_PGID) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}
#endif /* UK_LIBC_SYSCALLS */

#if UK_LIBC_SYSCALLS
pid_t tcgetpgrp(int fd __unused)
{
	/* We have a single "process group" */
	return UNIKRAFT_PGID;
}
#endif /* UK_LIBC_SYSCALLS */

#if UK_LIBC_SYSCALLS
int nice(int inc __unused)
{
	/* We don't support priority updates at the moment */
	errno = EPERM;
	return -1;
}
#endif /* UK_LIBC_SYSCALLS */

UK_SYSCALL_R_DEFINE(int, getpriority, int, which, id_t, who)
{
	int rc = 0;

	switch (which) {
	case PRIO_PROCESS:
	case PRIO_PGRP:
	case PRIO_USER:
		if (who == 0)
			/* Allow only for the calling "process" */
			rc = UNIKRAFT_PROCESS_PRIO;
		else {
			rc = -ESRCH;
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

UK_SYSCALL_R_DEFINE(int, setpriority, int, which, id_t, who, int, prio)
{
	int rc = 0;

	switch (which) {
	case PRIO_PROCESS:
	case PRIO_PGRP:
	case PRIO_USER:
		if (who == 0) {
			/* Allow only for the calling "process" */
			if (prio != UNIKRAFT_PROCESS_PRIO) {
				/* Allow setting only the default prio */
				rc = -EACCES;
			}
		} else {
			rc = -ESRCH;
		}
		break;
default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

UK_SYSCALL_R_DEFINE(int, prctl, int, option,
		    unsigned long, arg2,
		    unsigned long, arg3,
		    unsigned long, arg4,
		    unsigned long, arg5)
{
	UK_WARN_STUBBED();
	return 0; /* syscall has no effect */
}

UK_LLSYSCALL_R_DEFINE(int, prlimit64, int, pid, unsigned int, resource,
		      struct rlimit *, new_limit, struct rlimit *, old_limit)
{
	if (unlikely(pid != 0))
		uk_pr_debug("Do not support prlimit64 on PID %u, use current process\n",
			    pid);

	/*
	 * Lookup if resource can be set/retrieved
	 */
	switch (resource) {
	case RLIMIT_DATA:
	case RLIMIT_STACK:
	case RLIMIT_AS:
		break;
#if CONFIG_LIBVFSCORE
	case RLIMIT_NOFILE:
		break;
#endif
	default:
		uk_pr_err("Unsupported resource %u\n",
			  resource);
		return -EINVAL;
	}

	/*
	 * Set resource
	 */
	if (new_limit) {
		switch (resource) {
		default:
			uk_pr_err("Ignore updating resource %u: cur = %llu, max = %llu\n",
				  resource,
				  (unsigned long long) new_limit->rlim_cur,
				  (unsigned long long) new_limit->rlim_max);
			break;
		}
	}

	/*
	 * Get resource
	 */
	if (!old_limit)
		return 0;
	switch (resource) {
	case RLIMIT_STACK:
		old_limit->rlim_cur = __STACK_SIZE;
		old_limit->rlim_max = __STACK_SIZE;
		break;
	case RLIMIT_AS:
		old_limit->rlim_cur = RLIM_INFINITY;
		old_limit->rlim_max = RLIM_INFINITY;
		break;

#if CONFIG_LIBVFSCORE
	case RLIMIT_NOFILE:
		old_limit->rlim_cur = FDTABLE_MAX_FILES;
		old_limit->rlim_max = FDTABLE_MAX_FILES;
		break;
#endif

	default:
		break;
	}

	uk_pr_debug("Resource %u: cur = %llu, max = %llu\n",
		    resource,
		    (unsigned long long) old_limit->rlim_cur,
		    (unsigned long long) old_limit->rlim_max);
	return 0;
}

UK_SYSCALL_R_DEFINE(int, getrlimit, int, resource, struct rlimit *, rlim)
{
	return uk_syscall_r_prlimit64(0, (long) resource,
				      (long) NULL, (long) rlim);
}

UK_SYSCALL_R_DEFINE(int, setrlimit, int, resource, const struct rlimit *, rlim)
{
	return uk_syscall_r_prlimit64(0, (long) resource,
				      (long) rlim, (long) NULL);
}

UK_SYSCALL_R_DEFINE(int, getrusage, int, who,
		    struct rusage *, usage)
{
	if (!usage)
		return -EFAULT;

	memset(usage, 0, sizeof(*usage));
	return 0;
}

#if UK_LIBC_SYSCALLS
int prlimit(pid_t pid __unused, int resource, const struct rlimit *new_limit,
	    struct rlimit *old_limit)
{
	return uk_syscall_e_prlimit64(0, (long) resource,
				      (long) new_limit, (long) old_limit);
}
#endif /* UK_LIBC_SYSCALLS */
