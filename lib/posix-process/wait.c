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

#include <sys/types.h>
/* FIXME: #include <sys/wait.h> */
/* FIXME: #include <sys/siginfo.h> */
#include <uk/syscall.h>
#include <errno.h>
#include <stddef.h>

/* FIXME: Provide with sys/wait.h */
struct rusage;
typedef unsigned int idtype_t;
typedef unsigned int id_t;
typedef unsigned int siginfo_t;

UK_SYSCALL_R_DEFINE(pid_t, wait4, pid_t, pid,
		    int *, wstatus, int, options,
		    struct rusage *, rusage)
{
	return -ECHILD;
}

UK_SYSCALL_R_DEFINE(int, waitid, idtype_t, idtype, id_t, id,
		    siginfo_t *, infop, int, options)
{
	return -ECHILD;
}

#if UK_LIBC_SYSCALLS
pid_t wait3(int *wstatus, int options, struct rusage *rusage)
{
	return uk_syscall_e_wait4((long) -1, (long) wstatus,
				  (long) options, (long) rusage);
}

int waitpid(pid_t pid, int *wstatus, int options)
{
	return uk_syscall_e_wait4((long) pid, (long) wstatus,
				  (long) options, (long) NULL);
}

int wait(int *wstatus)
{
	return uk_syscall_e_wait4((long) -1, (long) wstatus,
				  (long) 0x0, (long) NULL);
}
#endif /* !UK_LIBC_SYSCALLS */
