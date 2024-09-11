/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/binfmt.h>
#include <uk/config.h>
#include <uk/event.h>
#include <uk/essentials.h>
#include <uk/plat/config.h>
#include <uk/process.h>
#include <uk/syscall.h>
#include <uk/thread.h>

#include "process.h"

void execve_arch_execenv_init(struct ukarch_execenv *execenv_new,
			      struct ukarch_execenv *execenv,
			      __uptr ip, __uptr sp);

UK_EVENT(POSIX_PROCESS_EXECVE_EVENT);

/* arg0: execenv_new, arg1: stack_old */
static void __noreturn execve_ctx_switch(long arg0, long arg1)
{
	struct ukarch_execenv *execenv_new;
	struct posix_thread *pthread_parent;
	struct posix_thread *pthread;
	struct uk_thread *this_thread;
	void *stack_old;

	execenv_new = (struct ukarch_execenv *)arg0;
	stack_old = (void *)arg1;

	this_thread = uk_thread_current();
	UK_ASSERT(this_thread);

	pthread = tid2pthread(ukthread2tid(this_thread)); /* FIXME */
	UK_ASSERT(pthread);

	pthread_parent = pthread->parent;

	/* If there is no parent, then must be calling execve()
	 * from init without having called vfork().
	 */
	if (!pthread_parent) {
		UK_ASSERT(pthread->process);
		UK_ASSERT(pthread->process->pid == 0);
		UK_ASSERT(pthread_parent->state != POSIX_THREAD_BLOCKED_VFORK);
		goto switch_ctx;
	}

	/* If we're coming from VFORK it's time to wake up the parent.
	 * Otherwise, free the old stack.
	 */
	if (pthread_parent->state == POSIX_THREAD_BLOCKED_VFORK) {
		uk_pr_debug("Waking up parent (tid %d)\n", pthread_parent->tid);
		uk_thread_wake(tid2ukthread(pthread_parent->tid));
		pthread_parent->state = POSIX_THREAD_RUNNING;
	} else {
		uk_free(this_thread->_mem.stack_a, stack_old);
	}

	uk_pr_debug("Switching context\n");
switch_ctx:
	ukarch_execenv_load((long)execenv_new);
}

/* Prepare process for executing new context. For a complete list see
 * "Effect on process attributes" in execve(2).
 */
static int pprocess_cleanup(struct uk_thread *thread __maybe_unused)
{
	/* Kill this thread's siblings */
	pprocess_kill_siblings(thread);

	return 0;
}

UK_LLSYSCALL_R_E_DEFINE(int, execve, const char *, pathname,
			char *const *, argv,
			char *const *, envp)
{
	struct posix_process_execve_event_data event_data;
	struct uk_binfmt_loader_args loader_args;
	struct ukarch_execenv *execenv_new;
	struct uk_thread *this_thread;
	struct ukarch_ctx ctx_old;
	void *stack_old;
	void *stack_new;
	int rc = 0;

	/* Linux deviates from POSIX by treating NULL pointers to
	 * argv / envp equivalently to passing a list with a single
	 * NULL element. Preserve this behavior for compatibiltiy
	 * and treat NULL pointers as valid.
	 */
	UK_ASSERT(pathname);

	this_thread = uk_thread_current();
	UK_ASSERT(this_thread);

	loader_args.argv = (const char **)argv;
	loader_args.envp = (const char **)envp;
	loader_args.pathname = (char *)pathname;
	loader_args.loader = NULL;
	loader_args.user = NULL;

	/* Assume that if argv is set the caller follows the convention */
	loader_args.progname = (argv && argv[0]) ? argv[0] : "<null>";

	/* Assign the default allocator to the loader. This will be used
	 * to allocate memory for the executable image.
	 */
	loader_args.alloc = uk_alloc_get_default();

	/* Allocate a new stack. Even if we don't come from vfork we
	 * can't operate on the current thread's stack without either
	 * corrupting it or wasting space. Use the threads's
	 * stack allocator for the new stack.
	 */
	stack_new = uk_malloc(this_thread->_mem.stack_a, STACK_SIZE);
	if (unlikely(!stack_new)) {
		uk_pr_err("Could not allocate stack\n");
		return -ENOMEM;
	}

	loader_args.stack_size = STACK_SIZE;
	loader_args.ctx.sp = ukarch_gen_sp((__uptr)stack_new,
					   loader_args.stack_size);

	uk_pr_debug("%s: New stack at %p - %p, stack pointer: %p\n",
		    loader_args.progname, stack_new,
		    (void *)((uintptr_t)stack_new + loader_args.stack_size),
		    (void *)loader_args.ctx.sp);

	/* Make room for the new execenv (context) to restore. Normally we
	 * would use the aux stack for this, and since this function is a
	 * noreturn we could even ovewrite the current auxsp frame. But
	 * just in case one day this is called outside the syscall context
	 * using the aux stack could corrupt the state of a syscall or the
	 * deferred exception handler. Avoid that by pushing instead into
	 * the newly allocated stack.
	 */
	loader_args.ctx.sp = ALIGN_DOWN(loader_args.ctx.sp,
					UKARCH_EXECENV_END_ALIGN);
	loader_args.ctx.sp -= UKARCH_EXECENV_SIZE;
	execenv_new = (struct ukarch_execenv *)loader_args.ctx.sp;

	/* Load executable */
	rc = uk_binfmt_load(&loader_args);
	if (unlikely(rc)) {
		uk_pr_err("%s: Unable to load (%d)\n", pathname, rc);
		goto err_free_stack_new;
	}

	/* Do arch-specific context (execenv) initialization */
	execve_arch_execenv_init(execenv_new, execenv,
				 loader_args.ctx.ip,
				 loader_args.ctx.sp);

	/* Prepare process for executing new context. First notify posix
	 * libraries that may have registered for the event, then do the
	 * internal cleanup.
	 */
	event_data.thread = this_thread;
	rc = uk_raise_event(POSIX_PROCESS_EXECVE_EVENT, &event_data);
	if (unlikely(rc)) {
		uk_pr_err("execve event error (%d)\n", rc);
		goto err_free_stack_new;
	}
	pprocess_cleanup(this_thread);

	/* Prepare switch to the new context.
	 *
	 * Update this thread's ctx and stack. We then need to free the old
	 * stack, and restore the new execenv (context). This needs to happen
	 * on the new stack, to avoid freeing the stack we are currently
	 * operating on, in case we are not on the auxstack. We do both via
	 * a context-switch trampoline that we execute on the new stack.
	 */
	stack_old = this_thread->_mem.stack;
	uk_pr_debug("%s: Old stack at %p - %p\n", loader_args.progname,
		    stack_old, (void *)((uintptr_t)stack_old + STACK_SIZE));

	this_thread->_mem.stack = stack_new;
	this_thread->ctx = loader_args.ctx;

	ukarch_ctx_init_entry2(&loader_args.ctx,
			       loader_args.ctx.sp,
			       1, /* keep regs */
			       execve_ctx_switch,
			       (long)execenv_new, (long)stack_old);
	ukarch_ctx_switch(&ctx_old, &loader_args.ctx);
	UK_BUG(); /* noreturn */

err_free_stack_new:
	uk_free(this_thread->_mem.stack_a, stack_new);

	return rc;
}
