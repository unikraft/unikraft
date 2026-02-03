/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "../../vmem.h"

#include <uk/assert.h>
#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/event.h>
#include <uk/lcpu.h>
#include <uk/print.h>

#include <string.h>

static int vmem_arch_pagefault(void *data)
{
	const char *faultstr[] __maybe_unused = {
		"read", "write", "exec"
	};
	struct uk_lcpu_except_err_ctx *ctx;
	unsigned int faulttype;
	unsigned long dfsc;
	struct uk_vas *vas;
	__vaddr_t vaddr;
	__u64 esr;
	int rc;

	ctx = (struct uk_lcpu_except_err_ctx *)data;
	UK_ASSERT(ctx);

	vaddr = (__vaddr_t)uk_lcpu_except_err_ctx_get_fault_addr(ctx);
	esr = uk_lcpu_arm64_except_err_ctx_get_esr(ctx);

	if (esr & UK_ARCH_ARM64_ESR_ISS_ABRT_WnR_BIT)
		faulttype = UK_VMA_FAULT_WRITE;
	else if (esr & UK_ARCH_ARM64_ESR_ISS_ABRT_ISV_BIT)
		faulttype = UK_VMA_FAULT_EXEC;
	else
		faulttype = UK_VMA_FAULT_READ;

	dfsc = esr & UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_MASK;
	if (dfsc >= UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L0 &&
	    dfsc <= UK_ARCH_ARM64_ESR_ISS_ABRT_FSC_TRANS_L3)
		faulttype |= UK_VMA_FAULT_NONPRESENT;
	else
		faulttype |= UK_VMA_FAULT_MISCONFIG;

	rc = vmem_pagefault(vaddr, faulttype,
			    (struct uk_lcpu_regs *)
			    uk_lcpu_except_err_ctx_get_regs(ctx));
	if (rc < 0) {
		vas = uk_vas_get_active();
		if (unlikely(vas && !(vas->flags & UK_VAS_FLAG_NO_PAGING)))
			uk_pr_debug("Cannot handle %s page fault at 0x%"
				    __PRIvaddr " (ec: 0x%lx): %s (%d).\n",
				    faultstr[faulttype & UK_VMA_FAULT_ACCESSTYPE],
				    vaddr, esr, strerror(-rc), -rc);
		uk_lcpu_except_err_ctx_set_handler_err(ctx, rc);
		return UK_EVENT_NOT_HANDLED;
	}

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UK_LCPU_EXCEPT_EVENT_ERR_PAGE_FAULT, vmem_arch_pagefault,
		      CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO);
