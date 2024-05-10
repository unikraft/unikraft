/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 * Copyright (c) 2021, NEC Laboratories Europe GmbH, NEC Corporation.
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

#include <uk/config.h>
#include <uk/arch/ctx.h>
#include <uk/plat/lcpu.h>
#if CONFIG_LIBUKDEBUG
#include <uk/assert.h>
#include <uk/print.h>
#else /* !CONFIG_LIBUKDEBUG */
#define UK_ASSERT(..) do {} while (0)
#define uk_pr_debug(..) do {} while (0)
#endif /* !CONFIG_LIBUKDEBUG */

/* Assembler functions */
void _ctx_x86_clearregs(void);
void _ctx_x86_call0(void);
void _ctx_x86_call1(void);
void _ctx_x86_call2(void);

void ukarch_ctx_init(struct ukarch_ctx *ctx,
		     __uptr sp, int keep_regs,
		     __uptr ip)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(ip);			/* NULL as IP will cause a crash */
	UK_ASSERT(!keep_regs && sp);	/* a stack is needed when clearing */

	_sp = ukarch_rstack_push(sp, (long) ip);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_call0);
	} else {
		_sp = ukarch_rstack_push(_sp, (long) _ctx_x86_call0);
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: start:%p sp:%p\n",
		    ctx, (void *) ip, (void *) sp);
}

void ukarch_ctx_init_entry0(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry0 entry)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	/* NOTE: System V calling convention for AMD64 requires on function
	 *       entrance that (stack pointer + 8) is 16-byte aligned:
	 * > The end of the input argument area shall be aligned on a 16 (32,
	 * > if __m256 is passed on stack) byte boundary. In other words, the
	 * > value (%rsp + 8) is always a multiple of 16 (32) when control is
	 * > transferred to the function entry point.
	 * https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf, chapter 3.2.2
	 *
	 * NOTE: Because our entry function prototypes are not using __m256
	 *       as data type, we can ignore 32-byte alignment.
	 * NOTE: The context epilogue functions `_ctx_x86_clearregs`,
	 *       `_ctx_x86_call0`, `_ctx_x86_call1`, and `_ctx_x86_call2`
	 *       do not require any alignments and will pass any sp offsets
	 *       through.
	 */
	sp  = ukarch_rstack_push(sp, (__u64) 0x0);
	_sp = ukarch_rstack_push(sp, (long) entry);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_call0);
	} else {
		_sp = ukarch_rstack_push(_sp, (long) _ctx_x86_call0);
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(), sp:%p\n",
		    ctx, entry, (void *) sp);
}

void ukarch_ctx_init_entry1(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry1 entry, long arg)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	sp  = ukarch_rstack_push(sp, (__u64) 0x0); /* SystemV call convention */
	_sp = ukarch_rstack_push(sp, (long) entry);
	_sp = ukarch_rstack_push(_sp, arg);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_call1);
	} else {
		_sp = ukarch_rstack_push(_sp, (long) _ctx_x86_call1);
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx), sp:%p\n",
		    ctx, entry, arg, (void *) sp);
}

void ukarch_ctx_init_entry2(struct ukarch_ctx *ctx,
			    __uptr sp, int keep_regs,
			    ukarch_ctx_entry2 entry, long arg0, long arg1)
{
	__uptr _sp;

	UK_ASSERT(ctx);
	UK_ASSERT(sp);			/* a stack is needed */
	UK_ASSERT(entry);		/* NULL as func will cause a crash */
	UK_ASSERT(!(sp & UKARCH_SP_ALIGN_MASK)); /* sp properly aligned? */

	sp  = ukarch_rstack_push(sp, (__u64) 0x0); /* SystemV call convention */
	_sp = ukarch_rstack_push(sp, (long) entry);
	_sp = ukarch_rstack_push(_sp, arg0);
	_sp = ukarch_rstack_push(_sp, arg1);
	if (keep_regs) {
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_call2);
	} else {
		_sp = ukarch_rstack_push(_sp, (long) _ctx_x86_call2);
		ukarch_ctx_init_bare(ctx, _sp, (long) _ctx_x86_clearregs);
	}

	uk_pr_debug("ukarch_ctx %p: entry:%p(%lx, %lx), sp:%p\n",
		    ctx, entry, arg0, arg1, (void *) sp);
}
