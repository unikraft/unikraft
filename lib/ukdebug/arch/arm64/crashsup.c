/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/crash.h>
#include <uk/essentials.h>

#include "../../crashdump.h"

extern __bool _uk_crash_explicit;

void uk_crash_populate_descr(struct uk_lcpu_except_err_ctx *ctx,
			     struct uk_crash_descr *descr)
{
	if (_uk_crash_explicit) {
		descr->reason = UK_CRASH_REASON_EXPLICIT;
		descr->uk_err = 0;
		return;
	}

	if (ctx->eid == ARM64_EXCEPTION_PAGE_FAULT) {
		descr->reason = UK_CRASH_REASON_PAGE_FAULT;
		descr->uk_err = ctx->handler_err;
		descr->arg1 = ctx->far;
		descr->arg2 = ctx->esr;
		return;
	}

	descr->reason = UK_CRASH_REASON_UNHANDLED_TRAP;
	descr->uk_err = 0;
	descr->arg1 = ctx->eid;
	descr->arg2 = (__u64)ctx->str;
	descr->arg3 = ctx->esr;
}
