/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "../../vmem.h"

#include <uk/arch/util.h>
#include <uk/arch/x86_64.h>
#include <uk/assert.h>
#include <uk/arch/types.h>
#include <uk/event.h>
#include <uk/print.h>
#include <uk/config.h>
#include <uk/lcpu.h>
#include <string.h>
#if CONFIG_LIBUKVMEM_TRACE_PAGEFAULTS
#include <uk/thread.h>
#endif

static int vmem_arch_pagefault(void *data)
{
	__u64 orig_gsbase = uk_arch_x86_64_enter_kernel_gsbase();
	struct uk_lcpu_except_err_ctx *ctx = data;
	__vaddr_t vaddr = (__vaddr_t)uk_lcpu_except_err_ctx_get_fault_addr(ctx);
	const char *faultstr[] __maybe_unused = {
		"read", "write", "exec"
	};
	unsigned int faulttype;
	struct uk_vas *vas;
	__u64 error_code;
	int rc;

	error_code = uk_lcpu_x86_64_except_err_ctx_get_error_code(ctx);
	if (error_code & UK_ARCH_X86_64_PF_EC_WR)
		faulttype = UK_VMA_FAULT_WRITE;
	else if (error_code & UK_ARCH_X86_64_PF_EC_ID)
		faulttype = UK_VMA_FAULT_EXEC;
	else
		faulttype = UK_VMA_FAULT_READ;

	if (!(error_code & UK_ARCH_X86_64_PF_EC_P))
		faulttype |= UK_VMA_FAULT_NONPRESENT;
	else if (error_code & UK_ARCH_X86_64_PF_EC_RSVD)
		faulttype |= UK_VMA_FAULT_MISCONFIG;

#if CONFIG_LIBUKVMEM_TRACE_PAGEFAULTS
	struct uk_thread *thread = uk_thread_current();
#endif
	rc = vmem_pagefault(vaddr, faulttype,
			    (struct uk_lcpu_regs *)
			    uk_lcpu_except_err_ctx_get_regs(ctx));
#if CONFIG_LIBUKVMEM_TRACE_PAGEFAULTS
	if (!rc && thread && (faulttype & UK_VMA_FAULT_NONPRESENT))
		thread->vmem_faults++;
#endif
	if (rc < 0) {
		vas = uk_vas_get_active();
		if (unlikely(uk_vas_paging_enabled(vas)))
			uk_pr_debug("Cannot handle %s page fault at 0x%"
				    __PRIvaddr " (ec: 0x%lx): %s (%d).\n",
				    faultstr[faulttype &
					    UK_VMA_FAULT_ACCESSTYPE],
				    vaddr,
				    error_code,
				    strerror(-rc), -rc);
		uk_lcpu_except_err_ctx_set_handler_err(ctx, rc);
		uk_arch_x86_64_exit_kernel_gsbase(orig_gsbase);
		return UK_EVENT_NOT_HANDLED;
	}

	uk_arch_x86_64_exit_kernel_gsbase(orig_gsbase);
	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT, vmem_arch_pagefault,
		      CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO);
