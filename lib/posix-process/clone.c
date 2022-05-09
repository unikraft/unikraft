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

#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <uk/process.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/arch/limits.h>
#include <uk/sched.h>

#include "process.h"

/* NOTE: From man pages about clone(2)
 *       [https://man7.org/linux/man-pages/man2/clone.2.html]:
 *       "The raw clone() system call corresponds more closely to fork(2)
 *        in that execution in the child continues from the point of the
 *        call.  As such, the fn and arg arguments of the clone() wrapper
 *        function are omitted.
 *
 *        In contrast to the glibc wrapper, the raw clone() system call
 *        accepts NULL as a stack argument (and clone3() likewise allows
 *        cl_args.stack to be NULL).  In this case, the child uses a
 *        duplicate of the parent's stack.  (Copy-on-write semantics ensure
 *        that the child gets separate copies of stack pages when either
 *        process modifies the stack.)  In this case, for correct
 *        operation, the CLONE_VM option should not be specified.  (If the
 *        child shares the parent's memory because of the use of the
 *        CLONE_VM flag, then no copy-on-write duplication occurs and chaos
 *        is likely to result.)
 *
 *        The order of the arguments also differs in the raw system call,
 *        and there are variations in the arguments across architectures,
 *        as detailed in the following paragraphs.
 *
 *        The raw system call interface on x86-64 and some other
 *        architectures (including sh, tile, and alpha) is:
 *
 *            long clone(unsigned long flags, void *stack,
 *                       int *parent_tid, int *child_tid,
 *                       unsigned long tls);
 *
 *        On x86-32, and several other common architectures (including
 *        score, ARM, ARM 64, PA-RISC, arc, Power PC, xtensa, and MIPS),
 *        the order of the last two arguments is reversed:
 *
 *            long clone(unsigned long flags, void *stack,
 *                       int *parent_tid, unsigned long tls,
 *                       int *child_tid);
 *       "
 */
static void _clone_child_gc(struct uk_thread *t)
{
	if (t->name) {
		free(DECONST(char *, t->name));
		t->name = NULL;
	}
}

/* Up to cl_args->tls, the fields of clone_args are required arguments */
#define CL_ARGS_REQUIRED_LEN					\
	(__offsetof(struct clone_args, tls)			\
	 + sizeof(((struct clone_args *)0)->tls))

static int _clone(struct clone_args *cl_args, size_t cl_args_len,
		  __uptr return_addr)
{
	struct uk_thread *t;
	struct uk_sched *s;
	struct uk_thread *child = NULL;
	__u64 flags;
	int ret;

	t = uk_thread_current();
	s = uk_sched_current();

	UK_ASSERT(s);
	UK_ASSERT(uk_syscall_return_addr());

	if (!cl_args || cl_args_len < CL_ARGS_REQUIRED_LEN) {
		uk_pr_debug("No or invalid clone arguments given\n");
		ret = -EINVAL;
		goto err_out;
	}

	flags = cl_args->flags;

#if UK_DEBUG
	uk_pr_debug("uk_syscall_r_clone(\n");
	uk_pr_debug(" flags: 0x%lx [", flags);
	if (flags & CLONE_NEWTIME)		uk_pr_debug(" NEWTIME");
	if (flags & CLONE_VM)			uk_pr_debug(" VM");
	if (flags & CLONE_FS)			uk_pr_debug(" FS");
	if (flags & CLONE_FILES)		uk_pr_debug(" FILES");
	if (flags & CLONE_SIGHAND)		uk_pr_debug(" SIGHAND");
	if (flags & CLONE_PIDFD)		uk_pr_debug(" PIDFD");
	if (flags & CLONE_PTRACE)		uk_pr_debug(" PTRACE");
	if (flags & CLONE_VFORK)		uk_pr_debug(" VFORK");
	if (flags & CLONE_PARENT)		uk_pr_debug(" PARENT");
	if (flags & CLONE_THREAD)		uk_pr_debug(" THREAD");
	if (flags & CLONE_NEWNS)		uk_pr_debug(" NEWNS");
	if (flags & CLONE_SYSVSEM)		uk_pr_debug(" SYSVSEM");
	if (flags & CLONE_SETTLS)		uk_pr_debug(" SETTLS");
	if (flags & CLONE_PARENT_SETTID)	uk_pr_debug(" PARENT_SETTID");
	if (flags & CLONE_CHILD_CLEARTID)	uk_pr_debug(" CHILD_CLEARTID");
	if (flags & CLONE_DETACHED)		uk_pr_debug(" DETACHED");
	if (flags & CLONE_UNTRACED)		uk_pr_debug(" UNTRACED");
	if (flags & CLONE_CHILD_SETTID)		uk_pr_debug(" CHILD_SETTID");
	if (flags & CLONE_NEWCGROUP)		uk_pr_debug(" NEWCGROUP");
	if (flags & CLONE_NEWUTS)		uk_pr_debug(" NEWUTS");
	if (flags & CLONE_NEWIPC)		uk_pr_debug(" NEWIPC");
	if (flags & CLONE_NEWUSER)		uk_pr_debug(" NEWUSER");
	if (flags & CLONE_NEWPID)		uk_pr_debug(" NEWPID");
	if (flags & CLONE_NEWNET)		uk_pr_debug(" NEWNET");
	if (flags & CLONE_IO)			uk_pr_debug(" IO");
	uk_pr_debug(" ]\n");
	if (flags & CLONE_PIDFD)
		uk_pr_debug(" pidfd: %d\n", (int) cl_args->pidfd);
	if (flags & CLONE_PARENT_SETTID)
		uk_pr_debug(" parent_tid: %p\n", (void *) cl_args->parent_tid);
	if (flags & (CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID))
		uk_pr_debug(" child_tid: %p\n", (void *) cl_args->child_tid);
	uk_pr_debug(" stack: %p\n", (void *) cl_args->stack);
	uk_pr_debug(" tls: %p\n", (void *) cl_args->tls);
	uk_pr_debug(" <return>: %p\n", (void *) return_addr);
	uk_pr_debug(")\n");
#endif /* UK_DEBUG */

	child = uk_thread_create_container(s->a,
					   NULL, 0, /* no stack */
					   s->a_uktls, /* UKTLS */
					   false,
					   (t->name) ? strdup(t->name) : NULL,
					   NULL,
					   _clone_child_gc);
	if (PTRISERR(child)) {
		ret = (PTR2ERR(child) != 0) ? PTR2ERR(child) : -ENOMEM;
		goto err_out;
	}

	/*
	 * Child starts at return address, sets given stack and given TLS:
	 * It starts at userland context...
	 */
	ukarch_ctx_init_entry0(&child->ctx,
			       (__uptr) cl_args->stack,
			       false,
			       (ukarch_ctx_entry0) return_addr);
	child->tlsp = (__uptr) cl_args->tls;
	uk_thread_set_runnable(child);

	uk_sched_thread_add(s, child);
	uk_sched_yield(); /* Enable the child to execute directly */

	return ukthread2tid(child);

err_out:
	return ret;
}

#if CONFIG_ARCH_X86_64
UK_LLSYSCALL_R_DEFINE(int, clone,
		      unsigned long, flags,
		      void *, sp,
		      int *, parent_tid,
		      int *, child_tid,
		      unsigned long, tlsp)
#else /* !CONFIG_ARCH_X86_64 */
UK_LLSYSCALL_R_DEFINE(int, clone,
		      unsigned long, flags,
		      void *, sp,
		      int *, parent_tid,
		      unsigned long, tlsp,
		      int *, child_tid)
#endif /* !CONFIG_ARCH_X86_64 */
{
	/* Translate */
	struct clone_args cl_args = {
		.flags       = (__u64) (flags & ~0xff),
		.pidfd       = (__u64) (flags & CLONE_PIDFD) ? parent_tid : 0,
		.child_tid   = (__u64) child_tid,
		.parent_tid  = (__u64) (flags & CLONE_PIDFD) ? 0 : parent_tid,
		.exit_signal = (__u64) (flags & 0xff),
		.stack       = (__u64) sp,
		.tls         = (__u64) tlsp
	};

	return _clone(&cl_args, sizeof(cl_args), uk_syscall_return_addr());
}

/* NOTE: There are currently no libc wrapper for clone3 */
UK_LLSYSCALL_R_DEFINE(long, clone3,
		      struct clone_args *, cl_args,
		      size_t, cl_args_len)
{
	return _clone(cl_args, cl_args_len, uk_syscall_return_addr());
}
