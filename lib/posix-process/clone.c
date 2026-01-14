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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <uk/arch/ctx.h>
#include <uk/plat/config.h>
#include <uk/process.h>
#include <uk/print.h>
#include <uk/syscall.h>
#include <uk/arch/limits.h>
#include <uk/sched.h>

#include "process.h"

#ifdef CONFIG_LIBUKDEBUG_ENABLE_ASSERT
#define CL_UKTLS_SANITY_MAGIC 0xb0b0f00d /* Bobo food */
static __thread uint32_t cl_uktls_magic = CL_UKTLS_SANITY_MAGIC;
#endif /* CONFIG_LIBUKDEBUG_ENABLE_ASSERT */

/* Up to cl_args->tls, the fields of clone_args are required arguments */
#define CL_ARGS_REQUIRED_LEN					\
	(__offsetof(struct clone_args, tls)			\
	 + sizeof(((struct clone_args *)0)->tls))

/*
 * NOTE: From man pages about clone(2)
 *       (https://man7.org/linux/man-pages/man2/clone.2.html):
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

/*
 * NOTE: The clone system call and the handling of the TLS
 *
 *       `uk_clone()` assumes that a passed TLS pointer is an Unikraft TLS.
 *       The only exception exists if `uk_clone()` is called from a context
 *       where a custom TLS is already active (depends on
 *       `CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS`). In such a case, an
 *       Unikraft TLS is allocated but the passed TLS pointer is activated.
 *       The reason is that Unikraft libraries place TLS variables and use
 *       the TLS effectively as TCB.
 *       In case no TLS is handed over (CLONE_SETTLS is not set), uk_clone will
 *       still allocate an Unikraft TLS but sets the TLS architecture pointer
 *       to zero.
 */
int uk_clone(struct clone_args *cl_args, size_t cl_args_len,
	     struct ukarch_execenv *execenv)
{
	struct posix_process_clone_event_data clone_event;
	struct posix_process *child_process __maybe_unused;
	struct posix_process *pprocess;
	struct posix_thread *pthread;
	struct uk_thread *child = NULL;
	struct uk_thread *t;
	struct uk_sched *s;
	pid_t child_tid;
	pid_t child_pid;
	__u64 stack_size;
	__u64 stack;
	__u64 flags;
	__u64 tls;
	int ret;

	t = uk_thread_current();
	s = uk_sched_current();

	UK_ASSERT(s);
	UK_ASSERT(t);
	/* Parent must have ECTX and a Unikraft TLS */
	UK_ASSERT((t->flags & UK_THREADF_ECTX)
		  && (t->flags & UK_THREADF_UKTLS));

	if (!cl_args || cl_args_len < CL_ARGS_REQUIRED_LEN) {
		uk_pr_debug("No or invalid clone arguments given\n");
		ret = -EINVAL;
		goto err_out;
	}

	/* shadow cl_args that may be modified
	 * by this function into locals
	 */
	stack_size = cl_args->stack_size;
	stack = cl_args->stack;
	flags = cl_args->flags;
	tls = cl_args->tls;

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
		uk_pr_debug(" pidfd: %d\n", (int)cl_args->pidfd);
	if (flags & CLONE_PARENT_SETTID)
		uk_pr_debug(" parent_tid: %p\n", (void *)cl_args->parent_tid);
	if (flags & (CLONE_CHILD_CLEARTID | CLONE_CHILD_SETTID))
		uk_pr_debug(" child_tid: %p\n", (void *)cl_args->child_tid);
	uk_pr_debug(" stack: %p\n", (void *)stack);
	uk_pr_debug(" tls: %p\n", (void *)tls);
	uk_pr_debug(" <return>: %p\n",
		    (void *)uk_lcpu_regs_get(execenv->regs,
						 PC));
	uk_pr_debug(")\n");
#endif /* UK_DEBUG */

#if !CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS
	if (unlikely(flags & !CLONE_THREAD)) {
		uk_pr_err("Multiprocess support not enabled\n");
		return -ENOTSUP;
	}
#endif /* !CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */

	if (unlikely(!(flags & CLONE_VM))) {
		uk_pr_err("CLONE_VM not set: Multiple address spaces are not supported\n");
		return -ENOTSUP;
	}

	if (unlikely(flags & CLONE_CHILD_SETTID && !cl_args->child_tid))
		return -EINVAL;

	if (unlikely(flags & CLONE_PARENT_SETTID && !cl_args->parent_tid))
		return -EINVAL;

	if (unlikely(flags & CLONE_DETACHED))
		uk_pr_warn("Ignoring historical CLONE_DETACHED\n");

	/* CLONE_VM requires that the child operates on the same memory
	 * space as the parent.
	 *
	 * Assign the parent's stack. We assign the user TLS to the
	 * parent's in clone_setup_child_ctx(). We copy the registers
	 * later below.
	 */
	if (flags & CLONE_VM) {
		if (!stack && !stack_size) {
			stack = uk_lcpu_regs_get(execenv->regs, SP);
			uk_pr_debug("Using parent's sp @ 0x%lx\n",
				    stack);
		}

		if (!tls) {
			tls = uk_lcpu_sysctx_get(execenv->sysctx, TLSP);
			uk_pr_debug("Using parent's tls @ 0x%lx\n",
				    tls);
		}
	}

	if ((flags & CLONE_SETTLS)
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	    && (uk_lcpu_sysctx_get(execenv->sysctx, TLSP) == 0x0)
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
	) {
		/* The caller already created a TLS for the child (for instance
		 * by a pthread API wrapper). We expect that this TLS is a
		 * Unikraft TLS.
		 */
		uk_pr_debug("Using passed TLS pointer %p as an Unikraft TLS\n",
			    (void *)tls);
		child = uk_thread_create_container2(s->a,
						    (__uptr)stack,
						    s->a_auxstack,
						    AUXSTACK_SIZE,
						    (__uptr)tls,
						    true, /* TLS is an UKTLS */
						    false, /* We want ECTX */
						    (t->name) ? strdup(t->name)
							      : NULL,
						    NULL,
						    _clone_child_gc);
	} else {
		/* If no TLS was given or the parent calls us already from
		 * a context with an userland TLS activated (kernel land vs.
		 * user land), we allocate an Unikraft TLS because Unikraft
		 * places TLS variables and uses them effectively as TCB.
		 */
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
		if (uk_lcpu_sysctx_get(execenv->sysctx, TLSP) != 0x0) {
			uk_pr_debug("Allocating an Unikraft TLS for the new child, parent called from context with custom TLS\n");
		} else
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
		{
			uk_pr_debug("Allocating an Unikraft TLS for the new child, no TLS given by parent\n");
		}
		child = uk_thread_create_container(s->a,
						   NULL, 0, /* Stack is given */
						   s->a_auxstack, AUXSTACK_SIZE,
						   s->a_uktls,
						   false, /* We want ECTX */
						   (t->name) ? strdup(t->name)
							     : NULL,
						   NULL,
						   _clone_child_gc);
	}
	if (PTRISERR(child)) {
		ret = (PTR2ERR(child) != 0) ? PTR2ERR(child) : -ENOMEM;
		goto err_out;
	}
#ifdef CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	/* Sanity check that the UKTLS of the child is really a Unikraft TLS:
	 * Do we find our magic on the TLS, is Bobo's banana there?
	 */
	UK_ASSERT(uk_thread_uktls_var(child, cl_uktls_magic)
		  == CL_UKTLS_SANITY_MAGIC);
#endif /* CONFIG_LIBUKDEBUG_ENABLE_ASSERT */

	if (flags & CLONE_VFORK) {
#if CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS
		/* We will be blocking the parent and pass control to the child
		 * via the scheduler. Therefore we need to set the child's TLS
		 * pointer the Unikraft TLS.
		 */
		child->tlsp = child->uktlsp;

		/* Since we didn't specify a stack to
		 * uk_thread_create_container2() above,
		 * we need to assign the stack manually.
		 */
		child->_mem.stack = (void *)stack;

		/* Also inherit the parent's stack allocator */
		child->_mem.stack_a = t->_mem.stack_a;
#else /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */
		ret = -ENOTSUP;
		goto err_free_child;
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */
	} else  {
		/* CLONE_SETTLS: Instead of just activating the Unikraft TLS
		 * we activate the passed TLS pointer as soon as the child
		 * wakes up.
		 * NOTE: If SETTLS is not set, we do not activate any TLS
		 * although a Unikraft TLS was allocated.
		 */
		if ((flags & CLONE_SETTLS))
			child->tlsp = tls;
		else
			child->tlsp = 0;
	}

	uk_pr_debug("Child is going to wake up with TLS pointer set to: %p (%s TLS)\n",
		    (void *) child->tlsp,
		    (child->tlsp != child->uktlsp) ? "custom" : "Unikraft");

	pprocess = uk_pprocess_current();
	UK_ASSERT(pprocess);

	if (cl_args->flags & CLONE_THREAD) {
		pthread = pprocess_create_pthread(pprocess, child);
		if (unlikely(PTRISERR(pthread))) {
			ret = PTR2ERR(pthread);
			uk_pr_err("Could not create pthread (%d)\n", ret);
			goto err_free_child;
		}
	} else {
#if CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS
		child_process = pprocess_create(uk_alloc_get_default(),
						child, t);
		if (unlikely(PTRISERR(child_process))) {
			ret = PTR2ERR(child_process);
			uk_pr_err("Could not create process (%d)\n", ret);
			goto err_free_child;
		}

		if (cl_args->exit_signal)
			child_process->exit_signal = cl_args->exit_signal;
		else
			child_process->exit_signal = SIGCHLD;
#else /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */
		ret = -ENOTSUP;
		goto err_free_child;
#endif /* CONFIG_LIBPOSIX_PROCESS_MULTIPROCESS */
	}
	child_tid = ukthread2tid(child);
	child_pid = ukthread2pid(child);

	/* Raise clone event */
	clone_event = (struct posix_process_clone_event_data) {
		.cl_args = cl_args,
		.cl_args_len = cl_args_len,
		.child = child,
		.parent = t,
		.ppid = pprocess->pid,
		.pid = child_pid,
		.tid = child_tid,
	};
	ret = pprocess_raise_clone_event(&clone_event);
	if (unlikely(ret < 0))
		goto err_free_child;

	uk_pr_debug("Thread cloned %p (%s) -> %p (%s): %d\n",
		    t, t->name ? child->name : "<unnamed>",
		    child, child->name ? child->name : "<unnamed>", ret);

	clone_setup_child_ctx(execenv, child, (__uptr)stack);

	uk_thread_set_runnable(child);

	/* Assign the child to the scheduler */
	ret = uk_sched_thread_add(s, child);
	if (unlikely(ret)) {
		uk_pr_err("Unable to add tid %d to scheduler (%d)\n",
			  child_tid, ret);
		goto err_free_child;
	}

	/* Can't fail past this point, update user parameters */
	if (flags & CLONE_CHILD_SETTID)
		*((pid_t *)cl_args->child_tid) = child_tid;

	if (flags & CLONE_PARENT_SETTID)
		*((pid_t *)cl_args->parent_tid) = child_tid;

	/* Set the return value depending on whether
	 * a thread or a process is created.
	 */
	if (cl_args->flags & CLONE_THREAD)
		ret = child_tid;
	else
		ret = child_pid;

	/* CLONE_VFORK: Block the parent until the child calls execve()
	 * or exit(). Yield to schedule the child.
	 */
	if (flags & CLONE_VFORK) {
		pthread = tid2pthread(ukthread2tid(t));
		uk_thread_block(t);
		pthread->state = POSIX_THREAD_BLOCKED_VFORK;
		uk_sched_yield();
		goto out;
	}

#ifdef CONFIG_LIBPOSIX_PROCESS_CLONE_PREFER_CHILD
	uk_sched_yield();
#endif /* CONFIG_LIBPOSIX_PROCESS_CLONE_PREFER_CHILD */
out:
	return ret;

err_free_child:
	/* also issues exit events */
	uk_thread_release(child);
err_out:
	return ret;
}

#if CONFIG_ARCH_X86_64
UK_LLSYSCALL_R_E_DEFINE(int, clone,
			unsigned long, flags,
			void *, sp,
			int *, parent_tid,
			int *, child_tid,
			unsigned long, tlsp)
#else /* !CONFIG_ARCH_X86_64 */
UK_LLSYSCALL_R_E_DEFINE(int, clone,
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
		.pidfd       = (__u64) ((flags & CLONE_PIDFD) ? parent_tid : 0),
		.child_tid   = (__u64) child_tid,
		.parent_tid  = (__u64) ((flags & CLONE_PIDFD) ? 0 : parent_tid),
		.exit_signal = (__u64) (flags & 0xff),
		.stack       = (__u64) sp,
		.tls         = (__u64) tlsp
	};

	return uk_clone(&cl_args, sizeof(cl_args), execenv);
}
