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
/* Derived from https://man7.org/linux/man-pages/man2/clone.2.html */

#ifndef _LINUX_SCHED_H_
#define _LINUX_SCHED_H_

#include <uk/config.h>
#include <uk/arch/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_LIBPOSIX_PROCESS_CLONE
/* Clone arguments for SYS_clone3
 * long syscall(SYS_clone3, struct clone_args *cl_args, size_t size);
 */
struct clone_args {
	__u64 flags;        /* Flags bit mask */
	__u64 pidfd;        /* Where to store PID file descriptor (int *) */
	__u64 child_tid;    /* Where to store child TID,
			     * in child's memory (pid_t *)
			     */
	__u64 parent_tid;   /* Where to store child TID,
			     * in parent's memory (pid_t *)
			     */
	__u64 exit_signal;  /* Signal to deliver to parent on termination */
	__u64 stack;        /* Pointer to lowest byte of stack */
	__u64 stack_size;   /* Size of stack */
	__u64 tls;          /* Location of new TLS */
	__u64 set_tid;      /* Pointer to a pid_t array (since Linux 5.5) */
	__u64 set_tid_size; /* Number of elements in set_tid */
	__u64 cgroup;       /* File descriptor for target cgroup
			     * of child (since Linux 5.7)
			     */
};
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE */

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_SCHED_H_ */
