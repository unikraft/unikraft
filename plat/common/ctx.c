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
 */

#include <stdint.h>
#include <stdlib.h>
#include <uk/plat/ctx.h>
#include <uk/plat/common/ctx.h>
#include <uk/assert.h>
#include <uk/plat/common/tls.h>
#include <uk/plat/common/cpu.h>

/* Gets run when a new thread is scheduled the first time ever,
 * defined in x86_[32/64].S
 */
extern void asm_thread_starter(void);

__sz ukplat_ctx_size(void)
{
	return sizeof(struct ukplat_ctx);
}

void ukplat_ctx_init(struct ukplat_ctx *sw_ctx,
		     unsigned long sp)
{
	UK_ASSERT(sw_ctx != NULL);

	sw_ctx->sp   = sp;
	sw_ctx->ip   = (unsigned long) asm_thread_starter;
}

extern void asm_ctx_start(unsigned long sp, unsigned long ip) __noreturn;

void ukplat_ctx_start(struct ukplat_ctx *sw_ctx)
{
	UK_ASSERT(sw_ctx != NULL);

	/* Switch stacks and run the thread */
	asm_ctx_start(sw_ctx->sp, sw_ctx->ip);

	UK_CRASH("Thread did not start.");
}

extern void asm_sw_ctx_switch(struct ukplat_ctx *prevctx,
			      struct ukplat_ctx *nextctx);

void ukplat_ctx_switch(struct ukplat_ctx *prevctx,
		       struct ukplat_ctx *nextctx)
{
	asm_sw_ctx_switch(prevctx, nextctx);
}

__uptr ukplat_tlsp_get(void)
{
	return (__uptr) get_tls_pointer();
}

void ukplat_tlsp_set(__uptr tlsp)
{
	set_tls_pointer(tlsp);
}
