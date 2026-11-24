/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <uk/arch/x86_64.h>
#include <uk/binfmt.h>
#include <uk/crash.h>
#include <uk/lcpu.h>
#include <uk/plat/config.h>
#include <uk/plat/native/ectx.h>
#include <uk/syscall.h>
#include <uk/vmem/process.h>

#include "process.h"
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
#include "signal/signal.h"
#endif

void execve_arch_execenv_init(struct ukarch_execenv *new,
			      struct ukarch_execenv *old, __uptr ip, __uptr sp);

static int exec_owned_check(struct posix_thread *self,
			    struct ukarch_execenv *frame)
{
	struct posix_process *process = self->process;
	struct uk_vas *vas = process->vas;
	struct posix_thread *thread, *next;
	__uptr sp = uk_lcpu_regs_get(frame->regs, SP);
	__uptr tls = uk_lcpu_sysctx_get(frame->sysctx, TLSP);
	unsigned long flags;
	int rc = 0;

	/* The saved user state and the kernel container must be independent. */
	if (!uk_vas_is_owned(vas) || uk_vas_get_active() != vas ||
	    sp < vas->range_start ||
	    sp >= vas->range_end || tls < vas->range_start ||
	    tls >= vas->range_end || self->thread->_mem.stack_a)
		return -ENOTSUP;
	flags = pprocess_lock_acquire();
	if (self->parent || self->tid != process->pid || process->exiting ||
	    process->state != POSIX_PROCESS_RUNNING)
		rc = -ENOTSUP;
	uk_pprocess_foreach_pthread(process, thread, next)
		if (thread != self)
			rc = -ENOTSUP;
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	if (uk_refcount_read(&process->signal->sigaction->refcnt) != 1)
		rc = -ENOTSUP;
#endif
	pprocess_lock_release(flags);
	return rc;
}

static int exec_owned_stack(struct uk_vas *vas, __sz size, __vaddr_t *top)
{
	__vaddr_t base;
	int rc;

	if (!size || size > vas->range_end - vas->range_start -
	    2 * UK_PAGING_PAGE_SIZE)
		return -E2BIG;
	base = vas->range_end - size - 2 * UK_PAGING_PAGE_SIZE;
	rc = uk_vma_map_anon(vas, &base, size + 2 * UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_NONE, 0, "exec stack");
	if (rc)
		return rc;
	rc = uk_vma_set_attr(vas, base + UK_PAGING_PAGE_SIZE, size,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0);
	if (!rc)
		*top = base + UK_PAGING_PAGE_SIZE + size;
	return rc;
}

static void exec_owned_signals(struct posix_process *process __unused)
{
#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	struct kern_sigaction *action;
	int signum;

	/* A private action table and the single calling thread were checked. */
	pprocess_signal_foreach(signum) {
		action = KERN_SIGACTION(process, signum);
		if (action->ks_handler != SIG_IGN)
			pprocess_signal_sigaction_clear(action);
	}
	process->signal->altstack = (stack_t){ .ss_flags = SS_DISABLE };
#endif
}

int pprocess_execve_owned(struct ukarch_execenv *execenv, const char *pathname,
			  char *const *argv, char *const *envp)
{
	struct posix_thread *self = uk_pthread_current();
	struct posix_process *process = self->process;
	struct uk_thread *current = self->thread;
	const __sz stack_size = pprocess_exec_stack_size(process);
	struct uk_vas *old = process->vas, *vas = NULL;
	struct uk_alloc *a = uk_vas_get_kernel()->a;
	struct pprocess_exec_args *args = NULL;
	struct uk_binfmt_loader_args loader = { 0 };
#if CONFIG_LIBPOSIX_PROCESS_BRK
	struct posix_process image = { ._a = a };
#endif
	struct posix_process_execve_event_data event;
	struct ukarch_execenv *frame;
	void *storage = NULL;
	__vaddr_t top;
	__uptr end;
	bool bound = false;
	unsigned long flags;
	int rc;

	rc = exec_owned_check(self, execenv);
	if (rc)
		return rc;
	uk_vas_get(old);
	rc = pprocess_exec_args_copy(a, old, (__vaddr_t)pathname,
				     (__vaddr_t)argv, (__vaddr_t)envp, &args);
	if (rc)
		goto out;
	rc = uk_vas_create_process(a, &vas);
	if (rc)
		goto out;
#if CONFIG_LIBPOSIX_PROCESS_BRK
	image.vas = vas;
	rc = pprocess_brk_map_space(&image);
	if (rc)
		goto out;
#endif
	rc = exec_owned_stack(vas, stack_size, &top);
	if (rc)
		goto out;
	storage = uk_malloc(a, UKARCH_EXECENV_SIZE + UKARCH_EXECENV_END_ALIGN);
	if (!storage) {
		rc = -ENOMEM;
		goto out;
	}
	end = ALIGN_UP((__uptr)storage + UKARCH_EXECENV_SIZE,
		       UKARCH_EXECENV_END_ALIGN);
	frame = (struct ukarch_execenv *)(end - UKARCH_EXECENV_SIZE);
	memset(frame, 0, sizeof(*frame));
	loader.alloc = a;
	loader.pathname = args->pathname;
	loader.progname = args->argc ? DECONST(char *, args->argv[0]) : "";
	loader.argv = args->argv;
	loader.envp = args->envp;
	loader.stack_size = stack_size;
	loader.ctx.sp = top;
	/* Only shared kernel storage is used until the new context is ready. */
	rc = uk_thread_set_vas(current, vas);
	if (rc)
		goto out;
	bound = true;
	rc = uk_binfmt_load(&loader);
	if (rc)
		goto out;
	rc = uk_binfmt_release_metadata(&loader);
	if (rc)
		goto out;
	execve_arch_execenv_init(frame, execenv, loader.ctx.ip, loader.ctx.sp);
	uk_plat_native_ectx_reset((struct uk_plat_native_ectx *)frame->ectx);
	uk_lcpu_sysctx_set(frame->sysctx, TLSP, 0);
	uk_lcpu_regs_set(frame->regs, RFLAGS, UK_ARCH_X86_64_RFLAGS_IF | 2);

	flags = pprocess_lock_acquire();
	if (process->exiting || process->state != POSIX_PROCESS_RUNNING) {
		pprocess_lock_release(flags);
		rc = -ESRCH;
		goto out;
	}
	/* Commit: allocation and image preparation have finished. */
	process->execed = true;
	pprocess_record_stack_size(process, stack_size);
	process->vas = vas;
	vas = NULL; /* Transfer the prepared reference to the process. */
#if CONFIG_LIBPOSIX_PROCESS_BRK
	process->brk_ctx.base = image.brk_ctx.base;
	process->brk_ctx.pos = 0;
	/* Keep the existing unlocked mutex of this single-threaded process. */
#endif
	exec_owned_signals(process);
	pprocess_lock_release(flags);
	(void)uk_syscall_r_static(SYS_set_tid_address, NULL);
	current->tlsp = 0;
	current->ctx = loader.ctx;
	uk_vas_put(old); /* Drop process ref; keep local pin. */
	event = (struct posix_process_execve_event_data) {
		.thread = current, .pid = process->pid, .tid = self->tid
	};
	rc = pprocess_raise_execve_event(&event);
	/* A late callback error cannot return to the former application. */
	if (rc < 0) {
		pprocess_exec_args_free(args);
		uk_free(a, storage);
		uk_vas_put(old);
		uk_syscall_r_static(SYS_exit_group, 127);
		UK_CRASH("Committed exec process failed to exit\n");
	}
	pprocess_exec_args_free(args);
	uk_vas_put(old);
	memcpy(execenv, frame, sizeof(*frame));
	uk_free(a, storage);
	ukarch_execenv_load((long)execenv);
	UK_BUG();
out:
	if (bound) {
		if (loader.loader)
			(void)uk_binfmt_unload(&loader);
		if (uk_thread_set_vas(current, old))
			UK_CRASH("Cannot restore exec caller\n");
	}
	uk_free(a, storage);
	pprocess_exec_args_free(args);
	uk_vas_put(vas);
	uk_vas_put(old);
	return rc;
}
