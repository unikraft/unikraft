/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Binary system call handler (Linux ABI)
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
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

#include <uk/syscall.h>
#include <uk/plat/syscall.h>
#include <uk/arch/ctx.h>
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
#include <uk/plat/tls.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
#include <uk/assert.h>
#include <uk/essentials.h>
#include "arch/regmap_linuxabi.h"
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#include <uk/plat/console.h> /* ukplat_coutk */
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

void ukplat_syscall_handler(struct __regs *r)
{
#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	struct uk_thread *self;
	__uptr orig_tlsp;
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
	/* Place backup of extended register state on stack */
	__sz ectx_align = ukarch_ectx_align();
	__u8 ectxbuf[ukarch_ectx_size() + ectx_align];
	struct ukarch_ectx *ectx = (struct ukarch_ectx *)
					 ALIGN_UP((__uptr) ectxbuf, ectx_align);
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
	char prsyscallbuf[512]; /* ANSI color is pretty hungry */
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	char prsyscallbuf[256];
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	int prsyscalllen;
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

	UK_ASSERT(r);

	/* Save extended register state */
	ukarch_ectx_sanitize(ectx);
	ukarch_ectx_store(ectx);

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	/* Activate Unikraft TLS */
	orig_tlsp = ukplat_tlsp_get();
	self = uk_thread_current();
	UK_ASSERT(self);
	ukplat_tlsp_set(self->uktlsp);
	_uk_syscall_ultlsp = orig_tlsp;
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

	/* uk_syscall6_r() will clear _uk_syscall_return_addr on return */
	_uk_syscall_return_addr = r->rip;

#if CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER
	_uk_printd(uk_libid_self(), __STR_BASENAME__, __LINE__,
			"Binary system call request \"%s\" (%lu) at ip:%p (arg0=0x%lx, arg1=0x%lx, ...)\n",
		    uk_syscall_name(r->rsyscall), r->rsyscall,
		    (void *) r->rip, r->rarg0, r->rarg1);
#endif /* CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER */
	r->rret0 = uk_syscall6_r(r->rsyscall,
				 r->rarg0, r->rarg1, r->rarg2,
				 r->rarg3, r->rarg4, r->rarg5);
#if CONFIG_LIBSYSCALL_SHIM_STRACE
	prsyscalllen = uk_snprsyscall(prsyscallbuf, ARRAY_SIZE(prsyscallbuf),
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
		     UK_PRSYSCALL_FMTF_ANSICOLOR | UK_PRSYSCALL_FMTF_NEWLINE,
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     UK_PRSYSCALL_FMTF_NEWLINE,
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     r->rsyscall, r->rret0, r->rarg0, r->rarg1, r->rarg2,
		     r->rarg3, r->rarg4, r->rarg5);
	/*
	 * FIXME:
	 * We directly use `ukplat_coutk()` until lib/ukdebug printing
	 * allows us to generate shortened output (avoiding list of details).
	 */
	ukplat_coutk(prsyscallbuf, (__sz) prsyscalllen);
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
	uk_thread_uktls_var(self, _uk_syscall_ultlsp) = 0x0;

	/* Restore original TLS only if it was _NOT_
	 * changed by the system call handler
	 */
	if (likely(ukplat_tlsp_get() == self->uktlsp)) {
		ukplat_tlsp_set(orig_tlsp);
	} else {
		uk_pr_debug("System call updated userland TLS pointer register to %p (before: %p)\n",
			    (void *) orig_tlsp, (void *) ukplat_tlsp_get());
	}
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

	/* Restore extended register state */
	ukarch_ectx_load(ectx);
}
