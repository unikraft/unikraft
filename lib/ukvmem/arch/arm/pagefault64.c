/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include "../../vmem.h"

#include <uk/assert.h>
#include <uk/arch/traps.h>
#include <uk/arch/types.h>
#include <uk/print.h>
#include <uk/config.h>
#include <string.h>

static int vmem_arch_pagefault(void *data)
{
	struct ukarch_trap_ctx *ctx = (struct ukarch_trap_ctx *)data;
	__vaddr_t vaddr = (__vaddr_t)ctx->far;
	const char *faultstr[] __maybe_unused = {
		"read", "write", "exec"
	};
	unsigned int faulttype;
	unsigned long dfsc;
	struct uk_vas *vas;
	int rc;

	if (ctx->esr & ARM64_PF_ESR_WnR)
		faulttype = UK_VMA_FAULT_WRITE;
	else if (ctx->esr & ARM64_PF_ESR_ISV)
		faulttype = UK_VMA_FAULT_EXEC;
	else
		faulttype = UK_VMA_FAULT_READ;

	dfsc = ctx->esr & ESR_ISS_ABRT_FSC_MASK;
	if (dfsc >= ESR_ISS_ABRT_FSC_TRANS_L0 &&
	    dfsc <= ESR_ISS_ABRT_FSC_TRANS_L3)
		faulttype |= UK_VMA_FAULT_NONPRESENT;
	else
		faulttype |= UK_VMA_FAULT_MISCONFIG;

	rc = vmem_pagefault(vaddr, faulttype, ctx->regs);
	if (unlikely(rc < 0)) {
		vas = uk_vas_get_active();
		if (unlikely(vas && !(vas->flags & UK_VAS_FLAG_NO_PAGING)))
			uk_pr_crit("Cannot handle %s page fault at 0x%"
				   __PRIvaddr" (ec: 0x%x): %s (%d).\n",
				   faultstr[faulttype &
					    UK_VMA_FAULT_ACCESSTYPE],
				   vaddr, ctx->error_code, strerror(-rc), -rc);

		return UK_EVENT_NOT_HANDLED;
	}

	return UK_EVENT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_PAGE_FAULT, vmem_arch_pagefault,
		      CONFIG_LIBUKVMEM_PAGEFAULT_HANDLER_PRIO);
