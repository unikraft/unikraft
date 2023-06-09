/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *          Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
 * Copyright (c) 2021, NEC Laboratories Europe GmbH. NEC Corporation.
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

#include <uk/arch/types.h>
#include <uk/plat/tls.h>
#include <uk/assert.h>

#if defined(LINUXUPLAT) && defined(__X86_64__)
#include <linuxu/x86/tls.h>
#elif defined(__X86_64__)
#include <x86/tls.h>
#elif defined(__ARM_64__)
#include <arm/arm64/tls.h>
#else
#error "For thread-local storage support, add tls.h for current architecture."
#endif

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS
#include <uk/plat/tls.h>
#include <uk/thread.h>
#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */

__uptr ukplat_tlsp_get(void)
{
	return (__uptr) get_tls_pointer();
}

void ukplat_tlsp_set(__uptr tlsp)
{
	set_tls_pointer(tlsp);
}

#if CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS

extern __uk_tls __uptr _uk_syscall_ultlsp;

__uptr ukplat_tlsp_enter(void)
{
	struct uk_thread *self = uk_thread_current();
	__uptr orig_tlsp = ukplat_tlsp_get();

	UK_ASSERT(self);
	ukplat_tlsp_set(self->uktlsp);
	_uk_syscall_ultlsp = orig_tlsp;
	return orig_tlsp;
}

void ukplat_tlsp_exit(__uptr orig_tlsp)
{
	struct uk_thread *self = uk_thread_current();

	UK_ASSERT(self);
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
}

#endif /* CONFIG_LIBSYSCALL_SHIM_HANDLER_ULTLS */
