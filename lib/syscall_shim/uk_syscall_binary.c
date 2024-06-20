/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Binary system call handler (Linux ABI)
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *          Sergiu Moga <sergiu@unikraft.io>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 * Copyright (c) 2024, Unikraft GmbH. All rights reserved.
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

#include <uk/syscall.h>
#include <uk/plat/syscall.h>
#include <uk/arch/ctx.h>
#include <uk/assert.h>
#include <uk/essentials.h>
#include <uk/thread.h>
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#if CONFIG_LIBUKCONSOLE
#include <uk/console.h>
#else /* !CONFIG_LIBUKCONSOLE */
#include <uk/print.h>
#endif /* !CONFIG_LIBUKCONSOLE */
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

/**
 * This is a convenience structure. The earlier architecture specific system
 * call entry call chain ensures that the execenv's of the caller is fully
 * stored, **BUT** it does not touch the current values so the C entry here
 * receiving this structure should ensure that the system registers are those
 * of Unikraft and not of the application.
 * This structure also contains `auxsp` because the previous caller needed
 * to know this value anyway in order to switch stacks so we can get that
 * value from here instead of re-fetching it from a system register. This
 * would have otherwise been costly, e.g. for x86 it could have meant a
 * wrmsr/rdgsbase which is significantly slower than a memory read so avoid
 * doing it again and choose the ugly design of adding this additional `auxsp`
 * field in favor of less CPU cycles wasted on the syscall hotpath.
 */
struct uk_syscall_ctx {
	struct ukarch_execenv execenv;
	__uptr auxsp;
};

void ukplat_syscall_handler(struct uk_syscall_ctx *usc)
{
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
	char prsyscallbuf[512]; /* ANSI color is pretty hungry */
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	char prsyscallbuf[256];
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	int prsyscalllen __maybe_unused;
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */
	struct ukarch_auxspcb *auxspcb;
	struct ukarch_execenv *execenv;
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	struct uk_thread *t;
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

	UK_ASSERT(usc);

	execenv = &usc->execenv;
	UK_ASSERT(execenv);

	/**
	 * The earlier architecture specific system call entry call chain
	 * ensures that the execenv's of the caller is fully stored, **BUT** it
	 * does not touch the current values so the C entry here receiving this
	 * uk_syscall_ctx structure should ensure that the **ACTIVE** system
	 * registers are those of Unikraft and not of the application.
	 */
	auxspcb = ukarch_auxsp_get_cb(usc->auxsp);
	ukarch_sysctx_load(&auxspcb->uksysctx);

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	t = uk_thread_current();
	UK_ASSERT(t);
	t->tlsp = t->uktlsp;
	UK_ASSERT(t->uktlsp == ukarch_auxspcb_get_uktlsp(auxspcb));
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

#if CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER
	_uk_printd(uk_libid_self(), __STR_BASENAME__, __LINE__,
			"Binary system call request \"%s\" (%lu) at ip:%p (arg0=0x%lx, arg1=0x%lx, ...)\n",
		    uk_syscall_name(execenv->regs.__syscall_rsyscall),
		    execenv->regs.__syscall_rsyscall,
		    (void *)execenv->regs.__syscall_rip,
		    execenv->regs.__syscall_rarg0,
		    execenv->regs.__syscall_rarg1);
#endif /* CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER */

	execenv->regs.__syscall_rret0 = uk_syscall6_r_e(execenv);

#if CONFIG_LIBSYSCALL_SHIM_STRACE
	prsyscalllen = uk_snprsyscall(prsyscallbuf, ARRAY_SIZE(prsyscallbuf),
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
		     UK_PRSYSCALL_FMTF_ANSICOLOR | UK_PRSYSCALL_FMTF_NEWLINE,
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     UK_PRSYSCALL_FMTF_NEWLINE,
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     execenv->regs.__syscall_rsyscall,
		     execenv->regs.__syscall_rret0,
		     execenv->regs.__syscall_rarg0,
		     execenv->regs.__syscall_rarg1,
		     execenv->regs.__syscall_rarg2,
		     execenv->regs.__syscall_rarg3,
		     execenv->regs.__syscall_rarg4,
		     execenv->regs.__syscall_rarg5);
	/*
	 * FIXME:
	 * Replace the call to `uk_pr_info` with a call to the kernel printing
	 * library once that exists. We also don't want to print all the meta
	 * data that `uk_pr_info` includes. Note also that right now, debug
	 * print calls also turn into a no-op if `ukconsole` is not available.
	 */
#if CONFIG_LIBUKCONSOLE
	uk_console_out(prsyscallbuf, (__sz) prsyscalllen);
#else /* !CONFIG_LIBUKCONSOLE */
	uk_pr_info(prsyscallbuf);
#endif /* !CONFIG_LIBUKCONSOLE */
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	t->tlsp = ukarch_sysctx_get_tlsp(&execenv->sysctx);
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
}
