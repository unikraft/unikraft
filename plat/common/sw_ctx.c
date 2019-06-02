/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Costin Lupu <costin.lupu@cs.pub.ro>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uk/plat/thread.h>
#include <uk/alloc.h>
#include <sw_ctx.h>
#include <uk/assert.h>
#include <tls.h>
#include <x86/cpu.h>

static void *sw_ctx_create(struct uk_alloc *allocator, unsigned long sp,
				unsigned long tlsp);
static void  sw_ctx_start(void *ctx) __noreturn;
static void  sw_ctx_switch(void *prevctx, void *nextctx);


/* Gets run when a new thread is scheduled the first time ever,
 * defined in x86_[32/64].S
 */
extern void asm_thread_starter(void);

static void *sw_ctx_create(struct uk_alloc *allocator, unsigned long sp,
				unsigned long tlsp)
{
	struct sw_ctx *ctx;
	size_t sz;

	UK_ASSERT(allocator != NULL);

	sz = ALIGN_UP(sizeof(struct sw_ctx), x86_cpu_features.extregs_align)
		+ x86_cpu_features.extregs_size;
	ctx = uk_malloc(allocator, sz);
	uk_pr_debug("Allocating %lu bytes for sw ctx at %p\n", sz, ctx);
	if (ctx == NULL) {
		uk_pr_warn("Error allocating software context.");
		return NULL;
	}

	ctx->sp = sp;
	ctx->tlsp = tlsp;
	ctx->ip = (unsigned long) asm_thread_starter;
	ctx->extregs = ALIGN_UP(((uintptr_t)ctx + sizeof(struct sw_ctx)),
				x86_cpu_features.extregs_align);
	// Initialize extregs area: zero out, then save a valid layout to it.
	memset((void *)ctx->extregs, 0, x86_cpu_features.extregs_size);
	save_extregs(ctx);

	return ctx;
}

extern void asm_ctx_start(unsigned long sp, unsigned long ip) __noreturn;

static void sw_ctx_start(void *ctx)
{
	struct sw_ctx *sw_ctx = ctx;

	UK_ASSERT(sw_ctx != NULL);

	set_tls_pointer(sw_ctx->tlsp);
	/* Switch stacks and run the thread */
	asm_ctx_start(sw_ctx->sp, sw_ctx->ip);

	UK_CRASH("Thread did not start.");
}

extern void asm_sw_ctx_switch(void *prevctx, void *nextctx);

static void sw_ctx_switch(void *prevctx, void *nextctx)
{
	struct sw_ctx *p = prevctx;
	struct sw_ctx *n = nextctx;

	save_extregs(p);
	restore_extregs(n);
	set_tls_pointer(n->tlsp);
	asm_sw_ctx_switch(prevctx, nextctx);
}

void sw_ctx_callbacks_init(struct ukplat_ctx_callbacks *ctx_cbs)
{
	UK_ASSERT(ctx_cbs != NULL);
	ctx_cbs->create_cb = sw_ctx_create;
	ctx_cbs->start_cb = sw_ctx_start;
	ctx_cbs->switch_cb = sw_ctx_switch;
}
