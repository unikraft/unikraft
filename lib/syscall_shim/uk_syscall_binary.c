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
#include <uk/assert.h>
#include <uk/essentials.h>
#include "arch/regmap_linuxabi.h"
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#include <uk/plat/console.h> /* ukplat_coutk */
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

void uk_signal_deliver(struct uk_syscall_ctx *usc);

void ukplat_syscall_handler(struct uk_syscall_ctx *usc)
{
#if CONFIG_LIBSYSCALL_SHIM_STRACE
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
	char prsyscallbuf[512]; /* ANSI color is pretty hungry */
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	char prsyscallbuf[256];
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
	int prsyscalllen;
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

	UK_ASSERT(usc);

	/* Save extended register state */
	ukarch_ectx_sanitize((struct ukarch_ectx *)&usc->ectx);
	ukarch_ectx_store((struct ukarch_ectx *)&usc->ectx);

	ukarch_sysregs_switch_uk(&usc->sysregs);

#if CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER
	_uk_printd(uk_libid_self(), __STR_BASENAME__, __LINE__,
			"Binary system call request \"%s\" (%lu) at ip:%p (arg0=0x%lx, arg1=0x%lx, ...)\n",
		    uk_syscall_name(usc->regs.rsyscall), usc->regs.rsyscall,
		    (void *)usc->regs.rip, usc->regs.rarg0,
		    usc->regs.rarg1);
#endif /* CONFIG_LIBSYSCALL_SHIM_DEBUG_HANDLER */

	usc->regs.rret0 = uk_syscall6_r_u(usc);

#if CONFIG_LIBSYSCALL_SHIM_STRACE
	prsyscalllen = uk_snprsyscall(prsyscallbuf, ARRAY_SIZE(prsyscallbuf),
#if CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR
		     UK_PRSYSCALL_FMTF_ANSICOLOR | UK_PRSYSCALL_FMTF_NEWLINE,
#else /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     UK_PRSYSCALL_FMTF_NEWLINE,
#endif /* !CONFIG_LIBSYSCALL_SHIM_STRACE_ANSI_COLOR */
		     usc->regs.rsyscall, usc->regs.rret0, usc->regs.rarg0,
		     usc->regs.rarg1, usc->regs.rarg2, usc->regs.rarg3,
		     usc->regs.rarg4, usc->regs.rarg5);
	/*
	 * FIXME:
	 * We directly use `ukplat_coutk()` until lib/ukdebug printing
	 * allows us to generate shortened output (avoiding list of details).
	 */
	ukplat_coutk(prsyscallbuf, (__sz) prsyscalllen);
#endif /* CONFIG_LIBSYSCALL_SHIM_STRACE */

#if CONFIG_LIBPOSIX_PROCESS_SIGNAL
	uk_signal_deliver(usc);
#endif /* CONFIG_LIBPOSIX_PROCESS_SIGNAL */

	ukarch_sysregs_switch_ul(&usc->sysregs);

	/* Restore extended register state */
	ukarch_ectx_load((struct ukarch_ectx *)&usc->ectx);
}
