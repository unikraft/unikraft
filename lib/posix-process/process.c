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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include "../posix-process/include/uk/process.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <uk/print.h>


int fork(void)
{
	/* fork() is not supported on this platform */
	errno = ENOSYS;
	return -1;
}

static
void exec_warn(const char *func,
		const char *path, char *const argv[], char *const envp[])
{
	int i;

	uk_pr_warn("%s(): path=%s", func, path);

	/* print arguments */
	i = 0;
	uk_pr_warn(" argv=[");
	while (argv[i]) {
		uk_pr_warn("%s%s", (i > 0 ? ", " : ""), argv[i]);
		i++;
	}
	uk_pr_warn("]");

	/* print environment variables */
	if (envp) {
		i = 0;
		uk_pr_warn(" envp=[");
		while (envp[i]) {
			uk_pr_warn("%s%s", (i > 0 ? ", " : ""), envp[i]);
			i++;
		}
		uk_pr_warn("]");
	}

	uk_pr_warn("\n");
}

int execve(const char *path, char *const argv[], char *const envp[])
{
	exec_warn(__func__, path, argv, envp);
	errno = ENOSYS;
	return -1;
}

int execv(const char *path, char *const argv[])
{
	exec_warn(__func__, path, argv, NULL);
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

int wait(int *status __unused)
{
	/* No children */
	errno = ECHILD;
	return -1;
}

pid_t waitpid(pid_t pid __unused, int *wstatus __unused, int options __unused)
{
	/* No children */
	errno = ECHILD;
	return -1;
}

pid_t wait3(int *wstatus __unused, int options __unused,
		struct rusage *rusage __unused)
{
	/* No children */
	errno = ECHILD;
	return -1;
}

pid_t wait4(pid_t pid __unused, int *wstatus __unused, int options __unused,
		struct rusage *rusage __unused)
{
	/* No children */
	errno = ECHILD;
	return -1;
}

int getpid(void)
{
	return UNIKRAFT_PID;
}

pid_t getppid(void)
{
	return UNIKRAFT_PPID;
}

pid_t setsid(void)
{
	/* We have a single "session" with a single "process" */
	errno = EPERM;
	return (pid_t) -1;
}

pid_t getsid(pid_t pid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		errno = ESRCH;
		return (pid_t) -1;
	}
	return UNIKRAFT_SID;
}

int setpgid(pid_t pid, pid_t pgid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		errno = ESRCH;
		return (pid_t) -1;
	}
	if (pgid != 0) {
		/* We have a single "group" with a single "process" */
		errno = EPERM;
		return (pid_t) -1;
	}
	return 0;
}

pid_t getpgid(pid_t pid)
{
	if (pid != 0) {
		/* We support only calls for the only calling "process" */
		errno = ESRCH;
		return (pid_t) -1;
	}
	return UNIKRAFT_PGID;
}

pid_t getpgrp(void)
{
	return UNIKRAFT_PGID;
}

int setpgrp(void)
{
	return setpgid(0, 0);
}

int tcsetpgrp(int fd __unused, pid_t pgrp)
{
	/* TODO check if fd is BADF */
	if (pgrp != UNIKRAFT_PGID) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

pid_t tcgetpgrp(int fd)
{
	/* We have a single "process group" */
	return UNIKRAFT_PGID;
}

int nice(int inc __unused)
{
	/* We don't support priority updates for unikernels */
	errno = EPERM;
	return -1;
}

int setpriority(int which __unused, id_t who __unused, int prio __unused)
{
	WARN_STUBBED();
	return 0;
}

int prctl(int option __unused, ...)
{
	WARN_STUBBED();
	return 0;
}
