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

#ifndef __UK_PROCESS_H__
#define __UK_PROCESS_H__

#include <uk/config.h>
#if CONFIG_LIBUKSCHED
#include <uk/thread.h>
#endif
#include <uk/prio.h>
#if CONFIG_LIBPOSIX_PROCESS_CLONE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>       /* CLONE_* constants */
#include <linux/sched.h> /* struct clone_args */

/* In case a libC is defining only a subset of our currently supported clone
 * flags, we provide here a completion of the list
 */
#ifndef CLONE_NEWTIME
#define CLONE_NEWTIME		0x00000080
#endif /* !CLONE_NEWTIME */
#ifndef CLONE_VM
#define CLONE_VM		0x00000100
#endif /* !CLONE_VM */
#ifndef CLONE_FS
#define CLONE_FS		0x00000200
#endif /* !CLONE_FS */
#ifndef CLONE_FILES
#define CLONE_FILES		0x00000400
#endif /* !CLONE_FILES */
#ifndef CLONE_SIGHAND
#define CLONE_SIGHAND		0x00000800
#endif /* !CLONE_SIGHAND */
#ifndef CLONE_PIDFD
#define CLONE_PIDFD		0x00001000
#endif /* !CLONE_PIDFD */
#ifndef CLONE_PTRACE
#define CLONE_PTRACE		0x00002000
#endif /* !CLONE_PTRACE */
#ifndef CLONE_VFORK
#define CLONE_VFORK		0x00004000
#endif /* !CLONE_VFORK */
#ifndef CLONE_PARENT
#define CLONE_PARENT		0x00008000
#endif /* !CLONE_PARENT */
#ifndef CLONE_THREAD
#define CLONE_THREAD		0x00010000
#endif /* !CLONE_THREAD */
#ifndef CLONE_NEWNS
#define CLONE_NEWNS		0x00020000
#endif /* !CLONE_NEWNS */
#ifndef CLONE_SYSVSEM
#define CLONE_SYSVSEM		0x00040000
#endif /* !CLONE_SYSVSEM */
#ifndef CLONE_SETTLS
#define CLONE_SETTLS		0x00080000
#endif /* !CLONE_SETTLS */
#ifndef CLONE_PARENT_SETTID
#define CLONE_PARENT_SETTID	0x00100000
#endif /* !CLONE_PARENT_SETTID */
#ifndef CLONE_CHILD_CLEARTID
#define CLONE_CHILD_CLEARTID	0x00200000
#endif /* !CLONE_CHILD_CLEARTID */
#ifndef CLONE_DETACHED
#define CLONE_DETACHED		0x00400000
#endif /* !CLONE_DETACHED */
#ifndef CLONE_UNTRACED
#define CLONE_UNTRACED		0x00800000
#endif /* !CLONE_UNTRACED */
#ifndef CLONE_CHILD_SETTID
#define CLONE_CHILD_SETTID	0x01000000
#endif /* !CLONE_CHILD_SETTID */
#ifndef CLONE_NEWCGROUP
#define CLONE_NEWCGROUP		0x02000000
#endif /* !CLONE_NEWCGROUP */
#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS		0x04000000
#endif /* !CLONE_NEWUTS */
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC		0x08000000
#endif /* !CLONE_NEWIPC */
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER		0x10000000
#endif /* !CLONE_NEWUSER */
#ifndef CLONE_NEWPID
#define CLONE_NEWPID		0x20000000
#endif /* !CLONE_NEWPID */
#ifndef CLONE_NEWNET
#define CLONE_NEWNET		0x40000000
#endif /* !CLONE_NEWNET */
#ifndef CLONE_IO
#define CLONE_IO		0x80000000
#endif /* !CLONE_IO */
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */

#if CONFIG_LIBUKSCHED
int uk_posix_process_create(struct uk_alloc *a,
			    struct uk_thread *thread,
			    struct uk_thread *parent);
void uk_posix_process_kill(struct uk_thread *thread);
#endif /* CONFIG_LIBUKSCHED */

#endif /* __UK_PROCESS_H__ */
